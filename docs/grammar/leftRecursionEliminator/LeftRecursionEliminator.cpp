#include "LeftRecursionEliminator.h"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void AssertIsNotEmpty(const raw::Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика не содержит правил для устранения левой рекурсии");
	}
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

void Substitute(raw::Rule& targetRule, const raw::Rule& sourceRule)
{
	raw::Alternatives newAlternatives;

	for (const auto& alt : targetRule.alternatives)
	{
		if (!alt.empty() && alt.front() == sourceRule.name)
		{
			for (const auto& sourceAlt : sourceRule.alternatives)
			{
				raw::Alternative newAlt;

				if (!(sourceAlt.size() == 1 && sourceAlt.front() == EMPTY_SYMBOL))
				{
					newAlt.insert(newAlt.end(), sourceAlt.begin(), sourceAlt.end());
				}

				newAlt.insert(newAlt.end(), alt.begin() + 1, alt.end());

				if (newAlt.empty())
				{
					newAlt.push_back(EMPTY_SYMBOL);
				}

				newAlternatives.push_back(std::move(newAlt));
			}
		}
		else
		{
			newAlternatives.push_back(alt);
		}
	}

	targetRule.alternatives = std::move(newAlternatives);
}

void EliminateDirectRecursion(raw::Rule& rule, const raw::Rules& existingRules, raw::Rules& additionalRules)
{
	raw::Alternatives alphas;
	raw::Alternatives betas;

	for (const auto& alt : rule.alternatives)
	{
		if (!alt.empty() && alt.front() == rule.name)
		{
			raw::Alternative alpha(alt.begin() + 1, alt.end());
			alphas.push_back(std::move(alpha));
		}
		else
		{
			betas.push_back(alt);
		}
	}

	if (alphas.empty())
	{
		return;
	}

	const std::string primeName = GenerateUniqueName(rule.name, existingRules, additionalRules);
	raw::Rule primeRule{primeName, {}};

	for (auto& alpha : alphas)
	{
		if (alpha.size() == 1 && alpha.front() == EMPTY_SYMBOL)
		{
			alpha.clear();
		}
		alpha.push_back(primeName);
		primeRule.alternatives.push_back(std::move(alpha));
	}
	primeRule.alternatives.push_back({EMPTY_SYMBOL});

	raw::Alternatives newBetas;
	if (betas.empty())
	{
		newBetas.push_back({primeName});
	}
	else
	{
		for (auto& beta : betas)
		{
			if (beta.size() == 1 && beta.front() == EMPTY_SYMBOL)
			{
				newBetas.push_back({primeName});
			}
			else
			{
				beta.push_back(primeName);
				newBetas.push_back(std::move(beta));
			}
		}
	}

	rule.alternatives = std::move(newBetas);
	additionalRules.push_back(std::move(primeRule));
}
} // namespace

LeftRecursionEliminator::LeftRecursionEliminator(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules LeftRecursionEliminator::Eliminate()
{
	AssertIsNotEmpty(m_rules);

	raw::Rules result = std::move(m_rules);
	raw::Rules additionalRules;

	for (size_t i = 0; i < result.size(); ++i)
	{
		for (size_t j = 0; j < i; ++j)
		{
			Substitute(result[i], result[j]);
		}

		EliminateDirectRecursion(result[i], result, additionalRules);
	}

	for (auto& newRule : additionalRules)
	{
		result.push_back(std::move(newRule));
	}

	return result;
}