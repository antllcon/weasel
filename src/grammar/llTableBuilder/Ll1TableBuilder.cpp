#include "Ll1TableBuilder.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

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
	std::vector<std::string> sorted(guides.begin(), guides.end());
	std::ranges::sort(sorted);

	std::string result;
	for (size_t i = 0; i < sorted.size(); ++i)
	{
		result += sorted[i];
		if (i + 1 < sorted.size())
		{
			result += ", ";
		}
	}
	return result;
}

std::unordered_map<std::string, size_t> CalculateRuleStartIndices(const Rules& rules)
{
	std::unordered_map<std::string, size_t> indices;
	size_t currentIndex = 1;

	for (const auto& rule : rules)
	{
		indices[rule.name] = currentIndex;
		currentIndex += rule.alternatives.size();
	}

	return indices;
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
	const auto ruleStartIndices = CalculateRuleStartIndices(m_rules);

	size_t dispatchIndex = 1;
	for (const auto& rule : m_rules)
	{
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			Ll1TableRow row;
			row.index = dispatchIndex++;
			row.guideSet = FormatGuideSet(rule.alternatives[i].guides);
			row.shift = false;
			row.stack = false;
			row.error = (i == rule.alternatives.size() - 1);
			row.finalState = false;
			row.transition = std::nullopt;
			table.push_back(std::move(row));
		}
	}

	size_t bodyIndex = dispatchIndex;
	size_t currentDispatchIndex = 1;

	for (const auto& rule : m_rules)
	{
		for (const auto& alt : rule.alternatives)
		{
			table[currentDispatchIndex - 1].transition = bodyIndex;
			currentDispatchIndex++;

			const size_t bodySize = alt.rule.empty() ? 1 : alt.rule.size();

			for (size_t i = 0; i < bodySize; ++i)
			{
				Ll1TableRow row;
				row.index = bodyIndex++;
				std::string symbol = alt.rule.empty() ? EMPTY_SYMBOL : alt.rule[i];
				const bool isLast = (i == bodySize - 1);

				if (symbol == EMPTY_SYMBOL)
				{
					row.guideSet = EMPTY_SYMBOL + " {" + FormatGuideSet(alt.guides) + "}";
					row.shift = false;
					row.transition = std::nullopt;
					row.stack = false;
					row.error = true;
					row.finalState = false;
					table.push_back(std::move(row));
				}
				else if (IsTerm(symbol) || symbol == END_SYMBOL)
				{
					row.guideSet = symbol;
					row.shift = true;
					row.transition = isLast ? std::nullopt : std::optional<size_t>(bodyIndex);
					row.stack = false;
					row.error = true;
					row.finalState = (symbol == END_SYMBOL);
					table.push_back(std::move(row));
				}
				else
				{
					row.guideSet = symbol;
					row.shift = false;

					const auto it = ruleStartIndices.find(symbol);
					AssertIsRuleFound(it != ruleStartIndices.end());
					row.transition = it->second;

					row.stack = true;
					row.error = true;
					row.finalState = false;
					table.push_back(std::move(row));

					if (isLast)
					{
						Ll1TableRow retRow;
						retRow.index = bodyIndex++;
						retRow.guideSet = "ret";
						retRow.shift = false;
						retRow.transition = std::nullopt;
						retRow.stack = false;
						retRow.error = true;
						retRow.finalState = false;
						table.push_back(std::move(retRow));
					}
				}
			}
		}
	}

	return table;
}