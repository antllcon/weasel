#include "LongRulesSplitter.h"
#include <string>
#include <stdexcept>

namespace
{
	void AssertIsGrammarNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Грамматика пуста, невозможно разбить длинные правила");
		}
	}

	bool IsLongAlternative(const raw::Alternative& alternative)
	{
		return alternative.size() > 2;
	}

	std::string GenerateNewNonTerminal(int& counter)
	{
		const std::string newName = "<N" + std::to_string(counter) + ">";
		++counter;
		return newName;
	}

	raw::Alternative SplitAlternative(
		const raw::Alternative& alternative,
		int& nextId,
		raw::Rules& generatedRules)
	{
		const std::string firstSymbol = alternative[0];
		std::string nextNonTerminal = GenerateNewNonTerminal(nextId);

		raw::Alternative replacement = {firstSymbol, nextNonTerminal};

		for (size_t i = 1; i < alternative.size() - 2; ++i)
		{
			std::string nextNextNonTerminal = GenerateNewNonTerminal(nextId);

			raw::Rule newRule = {nextNonTerminal, {{alternative[i], nextNextNonTerminal}}};
			generatedRules.push_back(std::move(newRule));

			nextNonTerminal = nextNextNonTerminal;
		}

		raw::Rule finalRule = {
			nextNonTerminal,
			{{alternative[alternative.size() - 2], alternative[alternative.size() - 1]}}
		};
		generatedRules.push_back(std::move(finalRule));

		return replacement;
	}
}

LongRulesSplitter::LongRulesSplitter(raw::Rules rules)
	: m_rules(std::move(rules))
	, m_nextId(1)
{
}

raw::Rules LongRulesSplitter::SplitLongRules()
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	raw::Rules generatedRules;

	for (auto& rule : m_rules)
	{
		raw::Alternatives newAlternatives;

		for (const auto& alternative : rule.alternatives)
		{
			if (IsLongAlternative(alternative))
			{
				raw::Alternative replacement = SplitAlternative(
					alternative,
					m_nextId,
					generatedRules
				);
				newAlternatives.push_back(std::move(replacement));
			}
			else
			{
				newAlternatives.push_back(alternative);
			}
		}

		rule.alternatives = std::move(newAlternatives);
	}

	for (auto& newRule : generatedRules)
	{
		m_rules.push_back(std::move(newRule));
	}

	return m_rules;
}