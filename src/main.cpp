#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llTableBuilder/Ll1TableBuilder.h"
#include "grammar/llTableBuilder/Ll1TablePrinter.h"
#include "grammar/parser/GrammarParser.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "grammar/tableBilder/Ll1Parser.h"
#include "grammar/tableBilder/ParseStepsPrinter.h"
#include <iostream>
#include <string>
#include <vector>
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

		GuideSetsCalculator calculator(result.rules, startSymbol);
		Rules rulesWithGuides = calculator.Calculate();
		PrintRulesWithGuides(rulesWithGuides, "Направляющие множества грамматик:");

		Ll1TableBuilder tableBuilder(rulesWithGuides);
		const auto ll1Table = tableBuilder.Build();
		Ll1TablePrinter::Print(ll1Table);

		std::string inputLine = "i(a)ta=b#";
		std::vector<std::string> tokens;
		for (char c : inputLine)
		{
			tokens.push_back(std::string(1, c));
		}

		Ll1Parser parser(ll1Table, startSymbol);
		auto steps = parser.Parse(tokens);

		ParseStepsPrinter::Print(steps, inputLine);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}