#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llTableBuilder/Ll1TableBuilder.h"
#include "grammar/llTableBuilder/Ll1TablePrinter.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

using namespace PrintGrammar;

namespace
{
bool IsFlagActive(OptimizationFlags value, OptimizationFlags flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

void PrintAppliedOptimizations(OptimizationFlags flags)
{
	if (flags == OptimizationFlags::None)
	{
		std::cout << "  - Исходная грамматика уже является LL(1) (Без оптимизаций)" << std::endl;
		return;
	}

	if (IsFlagActive(flags, OptimizationFlags::LeftFactorize))
	{
		std::cout << "  - Левая факторизация" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::EliminateLeftRecursion))
	{
		std::cout << "  - Устранение левой рекурсии" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnreachable))
	{
		std::cout << "  - Удаление недостижимых правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnproductive))
	{
		std::cout << "  - Удаление непродуктивных правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteUnitRules))
	{
		std::cout << "  - Удаление цепных правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteEmptyRules))
	{
		std::cout << "  - Удаление пустых правил" << std::endl;
	}
}
} // namespace

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::string startSymbol = "S";

		// raw::Rules rawRules = {
		// 	MakeRule("S", {{"E", "#"}}),
		// 	MakeRule("E", {{"E", "+", "T"}, {"E", "-", "T"}, {"T"}}),
		// 	MakeRule("T", {{"T", "*", "F"}, {"T", "/", "F"}, {"F"}}),
		// 	MakeRule("F", {{"I"}, {"I", "^", "N"}, {"(", "E", ")"}}),
		// 	MakeRule("I", {{"a"}, {"b"}, {"c"}, {"d"}}),
		// 	MakeRule("N", {{"2"}, {"3"}, {"4"}})};
		//
		// raw::Rules rawRules = {
		// 	MakeRule("S", {{"S", "a", "A", "b"}, {"S", "b"}, {"b", "A", "B", "a"}}),
		// 	MakeRule("A", {{"a", "c", "A", "b"}, {"c", "A"}, {"e"}}),
		// 	MakeRule("B", {{"b", "B"}, {"e"}})};

		raw::Rules rawRules = {
			MakeRule("S", {{"f", "A", "S", "d"}, {"e"}}),
			MakeRule("A", {{"A", "a"}, {"A", "b"}, {"d", "B"}, {"f"}}),
			MakeRule("B", {{"b", "c", "B"}, {"A"}})};

		PrintRules(rawRules, "Исходная грамматика:");

		const auto result = GrammarOptimizer::FindBestLL1(rawRules, startSymbol);

		std::cout << "Примененные оптимизации (с сохранением языка):" << std::endl;
		PrintAppliedOptimizations(result.flags);
		PrintRules(result.rules, "\nИтоговая грамматика:");

		if (result.isFound)
		{
			GuideSetsCalculator calculator(result.rules, startSymbol);
			Rules rulesWithGuides = calculator.Calculate();

			Ll1TableBuilder tableBuilder(rulesWithGuides);
			const auto ll1Table = tableBuilder.Build();
			Ll1TablePrinter::Print(ll1Table);

			std::cout << "[Result] Это LL(1) грамматика" << std::endl;
		}
		else
		{
			std::cout << "[Result] Это не LL(1) грамматика" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}