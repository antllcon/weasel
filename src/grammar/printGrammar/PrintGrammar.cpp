#include "PrintGrammar.h"

#include <iostream>

raw::Rule PrintGrammar::MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
}

void PrintGrammar::PrintRules(const raw::Rules& rules, const std::string& title)
{
	if (!title.empty())
	{
		std::cout << title << std::endl;
	}

	for (const auto& [name, alternatives] : rules)
	{
		std::cout << name << " -> ";
		for (size_t i = 0; i < alternatives.size(); ++i)
		{
			for (const auto& symbol : alternatives[i])
			{
				std::cout << symbol;
			}
			if (i + 1 < alternatives.size())
			{
				std::cout << " | ";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void PrintGrammar::PrintRulesWithGuides(const Rules& rules, const std::string& title)
{
	if (!title.empty())
	{
		std::cout << title << std::endl;
	}

	for (const auto& rule : rules)
	{
		std::cout << rule.name << " -> ";
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			const auto& alt = rule.alternatives[i];
			for (const auto& symbol : alt.rule)
			{
				std::cout << symbol;
			}

			std::cout << " { ";
			bool isFirst = true;
			for (const auto& term : alt.guides)
			{
				if (!isFirst)
				{
					std::cout << ", ";
				}
				std::cout << term;
				isFirst = false;
			}
			std::cout << " }";

			if (i + 1 < rule.alternatives.size())
			{
				std::cout << " | ";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}
