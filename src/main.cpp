#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llTableBuilder/Ll1TableBuilder.h"
#include "grammar/llTableBuilder/Ll1TablePrinter.h"
#include "grammar/parser/GrammarParser.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include <iostream>
#include <string>
#include <windows.h>

using namespace PrintGrammar;

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::filesystem::path grammarFile = "data.txt";
		raw::Rules rawRules = GrammarParser::ParseFile(grammarFile);
		const std::string startSymbol = rawRules.empty() ? "S" : rawRules.front().name;

		PrintRules(rawRules, "Исходная грамматика:");

		const auto result = GrammarOptimizer::FindBestLL1(rawRules, startSymbol);

		std::cout << "Примененные оптимизации (с сохранением языка):" << std::endl;
		PrintAppliedOptimizations(result.flags);
		PrintRules(result.rules, "Итоговая грамматика:");

		if (result.isFound)
		{
			GuideSetsCalculator calculator(result.rules, startSymbol);
			Rules rulesWithGuides = calculator.Calculate();
			PrintRulesWithGuides(rulesWithGuides, "Направляющие множества грамматик:");

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