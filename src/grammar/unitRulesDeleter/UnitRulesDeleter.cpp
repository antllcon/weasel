#include "UnitRulesDeleter.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace
{
void AssertIsNotEmpty(const raw::Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика не содержит правил для обработки");
	}
}

bool IsUnitAlternative(const raw::Alternative& alt)
{
	if (alt.size() != 1)
	{
		return false;
	}

	return !IsTerm(alt.front());
}

std::unordered_map<std::string, std::unordered_set<std::string>> BuildUnitPairs(const raw::Rules& rules)
{
	std::unordered_map<std::string, std::unordered_set<std::string>> unitPairs;

	for (const auto& rule : rules)
	{
		unitPairs[rule.name].insert(rule.name);
	}

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& rule : rules)
		{
			const std::string& sourceName = rule.name;

			for (const auto& alt : rule.alternatives)
			{
				if (IsUnitAlternative(alt))
				{
					const std::string& targetName = alt.front();

					for (const auto& reachableName : unitPairs[targetName])
					{
						if (unitPairs[sourceName].insert(reachableName).second)
						{
							changed = true;
						}
					}
				}
			}
		}
	}

	return unitPairs;
}

raw::Alternatives GetNonUnitAlternatives(const raw::Rules& rules, const std::string& targetRule)
{
	auto it = std::ranges::find_if(rules, [&targetRule](const raw::Rule& rule) {
		return rule.name == targetRule;
	});

	if (it == rules.end())
	{
		return {};
	}

	raw::Alternatives result;
	for (const auto& alt : it->alternatives)
	{
		if (!IsUnitAlternative(alt))
		{
			result.push_back(alt);
		}
	}

	return result;
}
} // namespace

UnitRulesDeleter::UnitRulesDeleter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules UnitRulesDeleter::DeleteUnitRules()
{
	AssertIsNotEmpty(m_rules);

	const auto unitPairs = BuildUnitPairs(m_rules);
	raw::Rules newRules;

	for (const auto& rule : m_rules)
	{
		raw::Rule newRule;
		newRule.name = rule.name;

		for (const auto& targetName : unitPairs.at(rule.name))
		{
			raw::Alternatives nonUnitAlts = GetNonUnitAlternatives(m_rules, targetName);

			for (auto& alt : nonUnitAlts)
			{
				if (std::ranges::find(newRule.alternatives, alt) == newRule.alternatives.end())
				{
					newRule.alternatives.push_back(std::move(alt));
				}
			}
		}

		if (!newRule.alternatives.empty())
		{
			newRules.push_back(std::move(newRule));
		}
	}

	return newRules;
}