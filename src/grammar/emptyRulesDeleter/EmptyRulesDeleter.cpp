#include "EmptyRulesDeleter.h"
#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

namespace
{
bool IsAllNullable(const raw::Alternative& alternative, const std::unordered_set<std::string>& nullables)
{
	if (alternative.size() == 1 && alternative.front() == EMPTY_SYMBOL)
	{
		return true;
	}

	for (const auto& symbol : alternative)
	{
		if (!nullables.contains(symbol))
		{
			return false;
		}
	}

	return true;
}

std::unordered_set<std::string> FindNullableSymbols(const raw::Rules& rules)
{
	std::unordered_set<std::string> nullables;
	bool changed = true;

	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			if (nullables.contains(rule.name))
			{
				continue;
			}

			for (const auto& alternative : rule.alternatives)
			{
				if (IsAllNullable(alternative, nullables))
				{
					nullables.insert(rule.name);
					changed = true;
					break;
				}
			}
		}
	}

	return nullables;
}

raw::Alternatives GenerateCombinations(const raw::Alternative& alternative, const std::unordered_set<std::string>& nullables)
{
	if (alternative.size() == 1 && alternative.front() == EMPTY_SYMBOL)
	{
		return {};
	}

	std::vector<size_t> nullableIndices;
	for (size_t i = 0; i < alternative.size(); ++i)
	{
		if (nullables.contains(alternative[i]))
		{
			nullableIndices.push_back(i);
		}
	}

	raw::Alternatives result;
	const size_t combinationsCount = 1ull << nullableIndices.size();

	for (size_t mask = 0; mask < combinationsCount; ++mask)
	{
		raw::Alternative newAlternative;
		size_t nullableIndexPos = 0;

		for (size_t i = 0; i < alternative.size(); ++i)
		{
			if (nullableIndexPos < nullableIndices.size() && nullableIndices[nullableIndexPos] == i)
			{
				if ((mask & (1ull << nullableIndexPos)) == 0)
				{
					newAlternative.push_back(alternative[i]);
				}
				nullableIndexPos++;
			}
			else
			{
				newAlternative.push_back(alternative[i]);
			}
		}

		if (!newAlternative.empty())
		{
			result.push_back(std::move(newAlternative));
		}
	}

	return result;
}
} // namespace

EmptyRulesDeleter::EmptyRulesDeleter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules EmptyRulesDeleter::DeleteEmptyRules()
{
	if (m_rules.empty())
	{
		return m_rules;
	}

	const std::unordered_set<std::string> nullables = FindNullableSymbols(m_rules);
	raw::Rules cleanedRules;

	for (auto& rule : m_rules)
	{
		raw::Alternatives newAlternatives;

		for (const auto& alternative : rule.alternatives)
		{
			raw::Alternatives combinations = GenerateCombinations(alternative, nullables);

			for (auto& comb : combinations)
			{
				if (std::ranges::find(newAlternatives, comb) == newAlternatives.end())
				{
					newAlternatives.push_back(std::move(comb));
				}
			}
		}

		if (!newAlternatives.empty())
		{
			rule.alternatives = std::move(newAlternatives);
			cleanedRules.push_back(std::move(rule));
		}
	}

	if (!cleanedRules.empty() && nullables.contains(cleanedRules[0].name))
	{
		const raw::Alternative emptyAlternative = {EMPTY_SYMBOL};
		if (std::ranges::find(cleanedRules[0].alternatives, emptyAlternative) == cleanedRules[0].alternatives.end())
		{
			cleanedRules[0].alternatives.push_back(emptyAlternative);
		}
	}

	return cleanedRules;
}