#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llValidator/LlValidator.h"
#include "timer/ScopedTimer.h"

#include <iostream>
#include <windows.h>

namespace
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
}

void PrintRules(const raw::Rules& rules, const std::string& title = "")
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

void PrintRulesWithGuides(const Rules& rules, const std::string& title = "")
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
} // namespace

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::string startSymbol = "P";

		raw::Rules rawRules = {
			MakeRule("P", {{"b", "d", ";", "X", "f"}}),
			MakeRule("X", {{"s", "Y"}, {"d", ";", "X"}}),
			MakeRule("Y", {{"e"}, {";", "s", "Y"}})};
		PrintRules(rawRules, "1. Исходная грамматика");

		auto optimRules = GrammarOptimizer::OptimizeForLL1(rawRules, startSymbol);
		PrintRules(optimRules, "2. Оптимизированная грамматика");

		GuideSetsCalculator calculator(optimRules, startSymbol);
		auto rulesWithGuides = calculator.Calculate();
		PrintRulesWithGuides(rulesWithGuides, "3. Грамматика с направляющими множествами");

		LlValidator validator(rulesWithGuides);
		if (validator.IsValid())
		{
			std::cout << "[Result] Грамматика LL(1)" << std::endl;
		}
		else
		{
			std::cout << "[Result] Грамматика не LL(1)" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}