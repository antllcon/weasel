#include "Ll1TableBuilder.h"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace
{
void AssertIsNotEmpty(const Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика пуста, построение таблицы невозможно");
	}
}

void AssertIsRuleFound(bool isFound)
{
	if (!isFound)
	{
		throw std::runtime_error("Не найдено правило для нетерминала при построении таблицы");
	}
}

std::string FormatGuideSet(const Guides& guides)
{
	std::vector sorted(guides.begin(), guides.end());
	std::ranges::sort(sorted);

	std::string result;
	for (size_t i = 0; i < sorted.size(); ++i)
	{
		result += sorted[i];
		if (i + 1 < sorted.size())
		{
			result += ",";
		}
	}
	return result;
}

Guides GetNonterminalGuides(const Rules& rules, const std::string& ntName)
{
	Guides result;
	for (const auto& rule : rules)
	{
		if (rule.name == ntName)
		{
			for (const auto& alt : rule.alternatives)
			{
				for (const auto& guide : alt.guides)
				{
					result.insert(guide);
				}
			}
			return result;
		}
	}

	AssertIsRuleFound(false);
	return result;
}

const Rule* FindRule(const Rules& rules, const std::string& name)
{
	for (const auto& r : rules)
	{
		if (r.name == name) return &r;
	}
	return nullptr;
}

std::unordered_map<std::string, size_t> BuildHeaderIndexMap(const Rules& rules)
{
	std::unordered_map<std::string, size_t> headerMap;
	size_t currentIndex = 1;

	for (const auto& rule : rules)
	{
		headerMap[rule.name] = currentIndex;
		currentIndex += rule.alternatives.size();
	}

	return headerMap;
}

std::map<std::pair<std::string, size_t>, size_t> BuildBodyIndexMap(const Rules& rules, size_t startIndex)
{
	std::map<std::pair<std::string, size_t>, size_t> bodyMap;
	size_t currentIndex = startIndex;

	for (const auto& rule : rules)
	{
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			bool isEpsilon = rule.alternatives[i].rule.empty() || (rule.alternatives[i].rule.size() == 1 && rule.alternatives[i].rule[0] == EMPTY_SYMBOL);

			if (!isEpsilon)
			{
				bodyMap[{rule.name, i}] = currentIndex;
				currentIndex += rule.alternatives[i].rule.size();
			}
		}
	}

	return bodyMap;
}

void AddHeaderRows(Ll1Table& table, const Rules& rules, const std::map<std::pair<std::string, size_t>, size_t>& bodyMap)
{
	for (const auto& rule : rules)
	{
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			Ll1TableRow row;
			row.index = table.size() + 1;
			row.name = rule.name;
			row.guides = FormatGuideSet(rule.alternatives[i].guides);
			row.error = (i == rule.alternatives.size() - 1);

			bool isEpsilon = rule.alternatives[i].rule.empty() || (rule.alternatives[i].rule.size() == 1 && rule.alternatives[i].rule[0] == EMPTY_SYMBOL);

			if (isEpsilon)
			{
				row.transition = std::nullopt;
			}
			else
			{
				row.transition = bodyMap.at({rule.name, i});
			}

			row.shift = false;
			row.stack = false;

			table.push_back(std::move(row));
		}
	}
}

void AddBodyRows(Ll1Table& table, const Rules& rules, const std::unordered_map<std::string, size_t>& headerMap, const std::map<std::pair<std::string, size_t>, size_t>& bodyMap)
{
	for (const auto& rule : rules)
	{
		for (const auto& alt : rule.alternatives)
		{
			bool isAltEpsilon = alt.rule.empty() || (alt.rule.size() == 1 && alt.rule[0] == EMPTY_SYMBOL);
			if (isAltEpsilon) continue;

			const size_t bodySize = alt.rule.size();

			for (size_t i = 0; i < bodySize; ++i)
			{
				Ll1TableRow row;
				row.index = table.size() + 1;

				std::string symbol = alt.rule[i];
				row.name = symbol;
				const bool isLast = (i == bodySize - 1);

				if (IsTerm(symbol) || symbol == END_SYMBOL)
				{
					row.guides = symbol;
					row.error = true;
					row.transition = isLast ? std::nullopt : std::optional(row.index + 1);
					row.shift = true;
					row.stack = false;
				}
				else
				{
					row.guides = FormatGuideSet(GetNonterminalGuides(rules, symbol));
					row.error = true;

					const auto it = headerMap.find(symbol);
					AssertIsRuleFound(it != headerMap.end());

					size_t targetTransition = it->second;

					const Rule* targetRule = FindRule(rules, symbol);
					if (targetRule && targetRule->alternatives.size() == 1)
					{
						bool isTargetEpsilon = targetRule->alternatives[0].rule.empty() || (targetRule->alternatives[0].rule.size() == 1 && targetRule->alternatives[0].rule[0] == EMPTY_SYMBOL);
						if (!isTargetEpsilon)
						{
							targetTransition = bodyMap.at({symbol, 0});
						}
					}

					row.transition = targetTransition;
					row.shift = false;
					row.stack = !isLast;
				}

				table.push_back(std::move(row));
			}
		}
	}
}
} // namespace

Ll1TableBuilder::Ll1TableBuilder(Rules rules)
	: m_rules(std::move(rules))
{
}

Ll1Table Ll1TableBuilder::Build() const
{
	AssertIsNotEmpty(m_rules);

	Ll1Table table;

	const auto headerMap = BuildHeaderIndexMap(m_rules);

	size_t totalHeaders = 0;
	for (const auto& [name, alternatives] : m_rules)
	{
		totalHeaders += alternatives.size();
	}

	const auto bodyMap = BuildBodyIndexMap(m_rules, totalHeaders + 1);

	AddHeaderRows(table, m_rules, bodyMap);
	AddBodyRows(table, m_rules, headerMap, bodyMap);

	return table;
}