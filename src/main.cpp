#include "grammar/CykLogger.h"
#include "grammar/CykParser.h"
#include "grammar/EmptyRulesDeleter.h"
#include "grammar/GrammarTypes.h"
#include "grammar/LongRulesSplitter.h"
#include "grammar/ProductiveRulesFilter.h"
#include "grammar/ReachableRulesFilter.h"
#include "grammar/TerminalsIsolator.h"
#include "grammar/UnitRulesDeleter.h"
#include <iostream>
#include <windows.h>

namespace
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
}

raw::Rules NormalizeToCnf(raw::Rules rules, const std::string& startSymbol)
{
	rules = EmptyRulesDeleter(std::move(rules)).DeleteEmptyRules();
	rules = UnitRulesDeleter(std::move(rules)).DeleteUnitRules();
	rules = LongRulesSplitter(std::move(rules)).SplitLongRules();
	rules = TerminalsIsolator(std::move(rules)).IsolateTerminals();
	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	return rules;
}

void PrintRules(const raw::Rules& rules, const std::string& title)
{
	std::cout << title << std::endl;
	for (const auto& [name, alternatives] : rules)
	{
		std::cout << name << " -> ";
		for (size_t i = 0; i < alternatives.size(); ++i)
		{
			for (const auto& symbol : alternatives[i])
			{
				std::cout << symbol << " ";
			}
			if (i + 1 < alternatives.size())
			{
				std::cout << "| ";
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

		const std::string startSymbol = "<S>";

		raw::Rules rawRules = {
			MakeRule("<S>", {
				{"a", "<S>", "b"},
				{"a", "b"}
			})
		};

		std::cout << "1. Исходная грамматика" << std::endl;
		PrintRules(rawRules, "");

		std::cout << "2. Нормализация (CNF)" << std::endl;
		raw::Rules cnfRules = NormalizeToCnf(rawRules, startSymbol);
		PrintRules(cnfRules, "Правила после CNF:");

		std::string input = "aabb";
		std::cout << "3. Парсинг строки: " << input << std::endl;

		CykParser parser(cnfRules, "<S>");
		auto result = parser.Parse(input);

		CykLogger::LogPyramid(result.table, input);

		if (result.isBelongsToLanguage) {
			std::cout << "[SUCCESS] Строка принадлежит языку" << std::endl;
		} else {
			std::cout << "[FAILURE] Строка не распознана" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[X] Ошибка: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}