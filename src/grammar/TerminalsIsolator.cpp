#include "TerminalsIsolator.h"
#include <stdexcept>

namespace
{
void AssertIsGrammarNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Грамматика пуста, невозможно изолировать терминалы");
	}
}

std::string GenerateTerminalWrapperName(const std::string& terminal)
{
	return "<T_" + terminal + ">";
}
}

TerminalsIsolator::TerminalsIsolator(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules TerminalsIsolator::IsolateTerminals()
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	raw::Rules newTerminalRules;

	for (auto& rule : m_rules)
	{
		for (auto& alternative : rule.alternatives)
		{
			if (alternative.size() < 2)
			{
				continue;
			}

			for (auto& symbol : alternative)
			{
				if (IsTerm(symbol))
				{
					if (!m_terminalCache.contains(symbol))
					{
						const std::string wrapperName = GenerateTerminalWrapperName(symbol);
						m_terminalCache[symbol] = wrapperName;

						raw::Rule newRule = {wrapperName, {{symbol}}};
						newTerminalRules.push_back(std::move(newRule));
					}

					symbol = m_terminalCache[symbol];
				}
			}
		}
	}

	for (auto& newRule : newTerminalRules)
	{
		m_rules.push_back(std::move(newRule));
	}

	return m_rules;
}