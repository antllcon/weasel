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

	rules = TerminalsIsolator(std::move(rules)).IsolateTerminals();
	rules = LongRulesSplitter(std::move(rules)).SplitLongRules();

	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	return rules;
}

void PrintRules(const raw::Rules& rules, const std::string& title)
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

		const std::string startSymbol = "<Z>";

		raw::Rules rawRules = {
			MakeRule("<Z>", {
				{"<E>", "+", "<T>"}
			}),
			MakeRule("<E>", {
				{"<E>"},
				{"<S>", "+", "<F>"},
				{"<T>"}
			}),
			MakeRule("<F>", {
				{"<F>"},
				{"<F>", "*", "<P>"},
				{"<P>"}
			}),
			MakeRule("<P>", {
				{"<G>"}
			}),
			MakeRule("<T>", {
				{"<T>", "+", "i"},
				{"i"},
				{"<T>", "*", "<F>"},
				{"<T>", "-", "<G>"}
			}),
			MakeRule("<G>", {
				{"<G>"},
				{"<G>", "<G>"},
				{"<F>"}
			}),
			MakeRule("<Q>", {
				{"<E>"},
				{"<E>", "+", "<F>"},
				{"<T>"},
				{"<S>"}
			}),
			MakeRule("<S>", {
				{"a"},
				{"d"}
			})
		};

		// raw::Rules rawRules = {
		// 	MakeRule("<S>", {
		// 		{"<A>"},
		// 		{"<B>"},
		// 		{"a"}
		// 	}),
		// 	MakeRule("<A>", {
		// 		{"a", "<B>"},
		// 		{"b", "<S>"}
		// 	}),
		// 	MakeRule("<B>", {
		// 		{"<A>", "<B>"},
		// 		{"<B>", "a"},
		// 		{"<A>", "d"},
		// 		{"<B>", "b"}
		// 	}),
		// 	MakeRule("<C>", {
		// 		{"<A>", "d"},
		// 		{"<B>", "b"},
		// 		{"<A>", "<B>"},
		// 		{"<B>", "a"},
		// 		{"a"}
		// 	})
		// };

		std::cout << "1. Исходная грамматика" << std::endl;
		PrintRules(rawRules, "");

		std::cout << "2. Нормализация (CNF)" << std::endl;
		raw::Rules cnfRules = NormalizeToCnf(rawRules, startSymbol);
		PrintRules(cnfRules, "Правила после CNF:");

		std::string input = "bbbcbaa";
		std::cout << "3. Парсинг строки: " << input << std::endl;

		CykParser parser(cnfRules, "<S>");
		auto result = parser.Parse(input);

		CykLogger::LogPyramid(result.table, input);

		if (result.isBelongsToLanguage)
		{
			std::cout << "[SUCCESS] Строка принадлежит языку" << std::endl;
		}
		else
		{
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