#include "UnitRulesDeleter.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace
{
	void AssertIsGrammarNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Грамматика пуста, невозможно удалить цепные правила");
		}
	}

	bool IsUnitAlternative(const raw::Alternative& alternative)
	{
		return alternative.size() == 1 && !IsTerm(alternative.front());
	}

	void CleanSelfReferencingRules(raw::Rules& rules)
	{
		for (auto& rule : rules)
		{
			std::erase_if(rule.alternatives, [&rule](const raw::Alternative& alt) {
				return IsUnitAlternative(alt) && alt.front() == rule.name;
			});
		}
	}

	std::optional<std::pair<size_t, std::string>> FindFirstUnitRule(const raw::Rules& rules)
	{
		for (size_t i = 0; i < rules.size(); ++i)
		{
			for (const auto& alt : rules[i].alternatives)
			{
				if (IsUnitAlternative(alt))
				{
					return std::make_pair(i, alt.front());
				}
			}
		}
		return std::nullopt;
	}

	raw::Alternatives GetAlternativesForSymbol(const raw::Rules& rules, const std::string& symbol)
	{
		for (const auto& rule : rules)
		{
			if (rule.name == symbol)
			{
				return rule.alternatives;
			}
		}
		return {};
	}

	void MergeAlternatives(raw::Alternatives& target, const raw::Alternatives& source)
	{
		for (const auto& srcAlt : source)
		{
			const bool exists = std::ranges::any_of(target, [&srcAlt](const raw::Alternative& tgtAlt) {
				return srcAlt == tgtAlt;
			});

			if (!exists)
			{
				target.push_back(srcAlt);
			}
		}
	}

	void ReplaceUnitRule(raw::Rules& rules, size_t ruleIndex, const std::string& targetSymbol)
	{
		auto& rule = rules[ruleIndex];

		std::erase_if(rule.alternatives, [&targetSymbol](const raw::Alternative& alt) {
			return IsUnitAlternative(alt) && alt.front() == targetSymbol;
		});

		raw::Alternatives targetAlternatives = GetAlternativesForSymbol(rules, targetSymbol);
		MergeAlternatives(rule.alternatives, targetAlternatives);
	}
}

UnitRulesDeleter::UnitRulesDeleter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules UnitRulesDeleter::DeleteUnitRules()
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	CleanSelfReferencingRules(m_rules);

	while (const auto unitRuleInfo = FindFirstUnitRule(m_rules))
	{
		const size_t ruleIndex = unitRuleInfo->first;
		const std::string targetSymbol = unitRuleInfo->second;

		ReplaceUnitRule(m_rules, ruleIndex, targetSymbol);
		CleanSelfReferencingRules(m_rules);
	}

	return m_rules;
}