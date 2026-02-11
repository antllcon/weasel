#include "ReachableRulesFilter.h"
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace
{
void AssertIsGrammarNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Грамматика пуста, невозможно определить стартовый нетерминал");
	}
}

void AddSymbolsFromAlternatives(const raw::Alternatives& alternatives, std::unordered_set<std::string>& reachableSet, bool& changed)
{
	for (const auto& alternative : alternatives)
	{
		for (const auto& symbol : alternative)
		{
			if (!reachableSet.contains(symbol))
			{
				reachableSet.insert(symbol);
				changed = true;
			}
		}
	}
}

std::unordered_set<std::string> FindReachableSymbols(const raw::Rules& rules)
{
	std::unordered_set<std::string> reachableSet;
	reachableSet.insert(rules.front().name);

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			if (reachableSet.contains(rule.name))
			{
				AddSymbolsFromAlternatives(rule.alternatives, reachableSet, changed);
			}
		}
	}

	return reachableSet;
}
}

ReachableRulesFilter::ReachableRulesFilter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules ReachableRulesFilter::FilterUnreachableRules() const
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	std::unordered_set<std::string> reachableSet = FindReachableSymbols(m_rules);

	raw::Rules filteredRules = m_rules;

	std::erase_if(filteredRules, [&reachableSet](const raw::Rule& rule) {
		return !reachableSet.contains(rule.name);
	});

	return filteredRules;
}