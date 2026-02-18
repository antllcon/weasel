#include "ReachableRulesFilter.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

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

std::unordered_set<std::string> FindReachableSymbols(const raw::Rules& rules, const std::string& startSymbol)
{
	std::unordered_set<std::string> reachableSet;
	reachableSet.insert(startSymbol);

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
} // namespace

ReachableRulesFilter::ReachableRulesFilter(raw::Rules rules, const std::string& startSymbol)
	: m_rules(std::move(rules))
	, m_startSymbol(startSymbol)
{
}

raw::Rules ReachableRulesFilter::FilterUnreachableRules() const
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	std::unordered_set<std::string> reachableSet = FindReachableSymbols(m_rules, m_startSymbol);

	raw::Rules filteredRules = m_rules;

	std::erase_if(filteredRules, [&reachableSet](const raw::Rule& rule) {
		return !reachableSet.contains(rule.name);
	});

	return filteredRules;
}