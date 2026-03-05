#include "LeftFactorizer.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace
{
void AssertIsNotEmpty(const raw::Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика не содержит правил для левой факторизации");
	}
}

bool IsValidPrefixSymbol(const std::string& symbol)
{
	return symbol != EMPTY_SYMBOL;
}

raw::Alternative FindLongestCommonPrefix(const raw::Alternatives& alts)
{
	raw::Alternative bestPrefix;

	for (size_t i = 0; i < alts.size(); ++i)
	{
		for (size_t j = i + 1; j < alts.size(); ++j)
		{
			raw::Alternative currentPrefix;
			size_t k = 0;

			while (k < alts[i].size() && k < alts[j].size() && alts[i][k] == alts[j][k] && IsValidPrefixSymbol(alts[i][k]))
			{
				currentPrefix.push_back(alts[i][k]);
				k++;
			}

			if (currentPrefix.size() > bestPrefix.size())
			{
				bestPrefix = std::move(currentPrefix);
			}
		}
	}

	return bestPrefix;
}

bool StartsWith(const raw::Alternative& alt, const raw::Alternative& prefix)
{
	if (alt.size() < prefix.size())
	{
		return false;
	}

	for (size_t i = 0; i < prefix.size(); ++i)
	{
		if (alt[i] != prefix[i])
		{
			return false;
		}
	}

	return true;
}

std::string GenerateUniqueName(const std::string& baseName, const raw::Rules& existingRules, const raw::Rules& newRules)
{
	std::string candidate = baseName;
	while (true)
	{
		if (!candidate.empty() && candidate.back() == '>')
		{
			candidate.insert(candidate.size() - 1, "'");
		}
		else
		{
			candidate += "'";
		}

		auto isTaken = [&candidate](const raw::Rule& r) { return r.name == candidate; };
		if (!std::ranges::any_of(existingRules, isTaken) && !std::ranges::any_of(newRules, isTaken))
		{
			return candidate;
		}
	}
}

bool FactorizeOnce(raw::Rule& rule, const raw::Rules& allRules, raw::Rules& newRules)
{
	raw::Alternative prefix = FindLongestCommonPrefix(rule.alternatives);
	if (prefix.empty())
	{
		return false;
	}

	raw::Alternatives factoredAlts;
	raw::Alternatives remainingAlts;

	for (auto& alt : rule.alternatives)
	{
		if (StartsWith(alt, prefix))
		{
			factoredAlts.push_back(std::move(alt));
		}
		else
		{
			remainingAlts.push_back(std::move(alt));
		}
	}

	const std::string primeName = GenerateUniqueName(rule.name, allRules, newRules);
	raw::Rule primeRule{primeName, {}};

	for (auto& alt : factoredAlts)
	{
		raw::Alternative suffix(alt.begin() + prefix.size(), alt.end());
		if (suffix.empty())
		{
			suffix.push_back(EMPTY_SYMBOL);
		}

		if (std::ranges::find(primeRule.alternatives, suffix) == primeRule.alternatives.end())
		{
			primeRule.alternatives.push_back(std::move(suffix));
		}
	}

	raw::Alternative newAlt = std::move(prefix);
	newAlt.push_back(primeName);
	remainingAlts.push_back(std::move(newAlt));

	rule.alternatives = std::move(remainingAlts);
	newRules.push_back(std::move(primeRule));

	return true;
}
} // namespace

LeftFactorizer::LeftFactorizer(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules LeftFactorizer::Factorize()
{
	AssertIsNotEmpty(m_rules);

	raw::Rules result = std::move(m_rules);
	bool changed = true;

	while (changed)
	{
		changed = false;
		raw::Rules newRules;

		for (auto& rule : result)
		{
			if (FactorizeOnce(rule, result, newRules))
			{
				changed = true;
			}
		}

		for (auto& newRule : newRules)
		{
			result.push_back(std::move(newRule));
		}
	}

	return result;
}