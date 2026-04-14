#include "ReachableRulesFilter.h"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_set>

namespace
{
void AssertIsNotEmpty(const raw::Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика не содержит правил для фильтрации");
	}
}

void AssertContainsStartSymbol(const raw::Rules& rules, const std::string& startSymbol)
{
	const bool found = std::ranges::any_of(rules, [&startSymbol](const raw::Rule& rule) {
		return rule.name == startSymbol;
	});

	if (!found)
	{
		throw std::runtime_error("Стартовый символ не найден в грамматике");
	}
}

std::unordered_set<std::string> FindReachableSymbols(const raw::Rules& rules, const std::string& startSymbol)
{
	std::unordered_set<std::string> reachable;
	std::queue<std::string> queue;

	reachable.insert(startSymbol);
	queue.push(startSymbol);

	while (!queue.empty())
	{
		std::string current = std::move(queue.front());
		queue.pop();

		auto it = std::ranges::find_if(rules, [&current](const raw::Rule& rule) {
			return rule.name == current;
		});

		if (it != rules.end())
		{
			for (const auto& alt : it->alternatives)
			{
				for (const auto& symbol : alt)
				{
					if (!IsTerm(symbol) && !reachable.contains(symbol))
					{
						reachable.insert(symbol);
						queue.push(symbol);
					}
				}
			}
		}
	}

	return reachable;
}
} // namespace

ReachableRulesFilter::ReachableRulesFilter(raw::Rules rules, std::string_view startSymbol)
	: m_rules(std::move(rules))
	, m_startSymbol(std::move(startSymbol))
{
}

raw::Rules ReachableRulesFilter::FilterUnreachableRules()
{
	AssertIsNotEmpty(m_rules);
	AssertContainsStartSymbol(m_rules, m_startSymbol);

	const std::unordered_set<std::string> reachable = FindReachableSymbols(m_rules, m_startSymbol);
	raw::Rules filteredRules;

	for (auto& rule : m_rules)
	{
		if (reachable.contains(rule.name))
		{
			filteredRules.push_back(std::move(rule));
		}
	}

	return filteredRules;
}