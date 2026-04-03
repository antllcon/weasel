#include "GuideSetsCalculator.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace
{
using SetMap = std::unordered_map<std::string, std::unordered_set<std::string>>;

void AssertIsNotEmpty(const raw::Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика пуста, невозможно вычислить направляющие множества");
	}
}

std::unordered_set<std::string> GetSequenceFirst(const raw::Alternative& alt, const SetMap& firstSets)
{
	std::unordered_set<std::string> result;
	bool allContainEmpty = true;

	for (const auto& symbol : alt)
	{
		if (IsTerm(symbol))
		{
			result.insert(symbol);
			allContainEmpty = false;
			break;
		}

		if (symbol == EMPTY_SYMBOL)
		{
			continue;
		}

		const auto& symbolFirst = firstSets.at(symbol);
		for (const auto& term : symbolFirst)
		{
			if (term != EMPTY_SYMBOL)
			{
				result.insert(term);
			}
		}

		if (!symbolFirst.contains(EMPTY_SYMBOL))
		{
			allContainEmpty = false;
			break;
		}
	}

	if (allContainEmpty || (alt.size() == 1 && alt.front() == EMPTY_SYMBOL))
	{
		result.insert(EMPTY_SYMBOL);
	}

	return result;
}

SetMap CalculateFirstSets(const raw::Rules& rules)
{
	SetMap firstSets;
	for (const auto& rule : rules)
	{
		firstSets[rule.name] = {};
	}

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			for (const auto& alt : rule.alternatives)
			{
				auto altFirst = GetSequenceFirst(alt, firstSets);
				for (const auto& term : altFirst)
				{
					if (firstSets[rule.name].insert(term).second)
					{
						changed = true;
					}
				}
			}
		}
	}

	return firstSets;
}

SetMap CalculateFollowSets(const raw::Rules& rules, const SetMap& firstSets, const std::string& startSymbol)
{
	SetMap followSets;
	for (const auto& rule : rules)
	{
		followSets[rule.name] = {};
	}

	followSets[startSymbol].insert(END_SYMBOL);

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			for (const auto& alt : rule.alternatives)
			{
				for (size_t i = 0; i < alt.size(); ++i)
				{
					const std::string& symbol = alt[i];
					if (IsTerm(symbol) || symbol == EMPTY_SYMBOL)
					{
						continue;
					}

					raw::Alternative remainder(alt.begin() + i + 1, alt.end());
					auto remainderFirst = GetSequenceFirst(remainder, firstSets);

					for (const auto& term : remainderFirst)
					{
						if (term != EMPTY_SYMBOL)
						{
							if (followSets[symbol].insert(term).second)
							{
								changed = true;
							}
						}
					}

					if (remainderFirst.contains(EMPTY_SYMBOL) || remainder.empty())
					{
						for (const auto& term : followSets[rule.name])
						{
							if (followSets[symbol].insert(term).second)
							{
								changed = true;
							}
						}
					}
				}
			}
		}
	}

	return followSets;
}
} // namespace

GuideSetsCalculator::GuideSetsCalculator(raw::Rules rules, std::string startSymbol)
	: m_rules(std::move(rules))
	, m_startSymbol(std::move(startSymbol))
{
}

Rules GuideSetsCalculator::Calculate() const
{
	AssertIsNotEmpty(m_rules);

	const SetMap firstSets = CalculateFirstSets(m_rules);
	const SetMap followSets = CalculateFollowSets(m_rules, firstSets, m_startSymbol);

	Rules result;
	for (const auto& rawRule : m_rules)
	{
		Rule newRule;
		newRule.name = rawRule.name;

		for (const auto& rawAlt : rawRule.alternatives)
		{
			Alternative newAlt;
			newAlt.rule = rawAlt;

			auto altFirst = GetSequenceFirst(rawAlt, firstSets);

			for (const auto& term : altFirst)
			{
				if (term != EMPTY_SYMBOL)
				{
					newAlt.guides.insert(term);
				}
			}

			if (altFirst.contains(EMPTY_SYMBOL))
			{
				for (const auto& term : followSets.at(rawRule.name))
				{
					newAlt.guides.insert(term);
				}
			}

			newRule.alternatives.push_back(std::move(newAlt));
		}
		result.push_back(std::move(newRule));
	}

	return result;
}