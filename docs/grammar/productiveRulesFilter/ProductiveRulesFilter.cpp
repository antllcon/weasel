#include "ProductiveRulesFilter.h"
#include <algorithm>
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

bool IsAlternativeProductive(const raw::Alternative& alt, const std::unordered_set<std::string>& productiveSet)
{
	if (alt.size() == 1 && alt.front() == EMPTY_SYMBOL)
	{
		return true;
	}

	for (const auto& symbol : alt)
	{
		if (!IsTerm(symbol) && !productiveSet.contains(symbol))
		{
			return false;
		}
	}

	return true;
}

std::unordered_set<std::string> FindProductiveSymbols(const raw::Rules& rules)
{
	std::unordered_set<std::string> productiveSet;
	bool changed = true;

	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			if (productiveSet.contains(rule.name))
			{
				continue;
			}

			for (const auto& alt : rule.alternatives)
			{
				if (IsAlternativeProductive(alt, productiveSet))
				{
					productiveSet.insert(rule.name);
					changed = true;
					break;
				}
			}
		}
	}

	return productiveSet;
}
} // namespace

ProductiveRulesFilter::ProductiveRulesFilter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules ProductiveRulesFilter::FilterUnproductiveRules()
{
	AssertIsNotEmpty(m_rules);

	const std::unordered_set<std::string> productiveSet = FindProductiveSymbols(m_rules);
	raw::Rules filteredRules;

	for (auto& rule : m_rules)
	{
		if (!productiveSet.contains(rule.name))
		{
			continue;
		}

		raw::Alternatives validAlternatives;
		for (auto& alt : rule.alternatives)
		{
			if (IsAlternativeProductive(alt, productiveSet))
			{
				validAlternatives.push_back(std::move(alt));
			}
		}

		if (!validAlternatives.empty())
		{
			rule.alternatives = std::move(validAlternatives);
			filteredRules.push_back(std::move(rule));
		}
	}

	return filteredRules;
}