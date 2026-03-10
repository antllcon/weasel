#include "grammar/GrammarTypes.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/leftFactorizer/LeftFactorizer.h"
#include "grammar/leftRecursionEliminator/LeftRecursionEliminator.h"
#include "grammar/llValidator/LlValidator.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "grammar/productiveRulesFilter/ProductiveRulesFilter.h"
#include "grammar/reachableRulesFilter/ReachableRulesFilter.h"
#include "grammar/tableBilder/Ll1Parser.h"
#include "grammar/tableBilder/ParseTablePrinter.h"
#include "grammar/tableBilder/TableBuilder.h"
#include <iostream>
#include <windows.h>

using namespace PrintGrammar;

raw::Rules OptimizeArithmeticGrammar(raw::Rules rules, const std::string& startSymbol)
{
	rules = LeftFactorizer(std::move(rules)).Factorize();
	PrintRules(rules, "Сделал факторизацию");

	rules = LeftRecursionEliminator(std::move(rules)).Eliminate();
	PrintRules(rules, "Убрал левую рекурсию");

	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	PrintRules(rules, "Убрал непродуктивные правила");

	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	PrintRules(rules, "Убрал недостижимые правила");

	return rules;
}

std::vector<std::string> Tokenize(const std::string& input)
{
	std::vector<std::string> tokens;

	for (char c : input)
	{
		tokens.emplace_back(1, c);
	}

	return tokens;
}

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::string startSymbol = "S";

		raw::Rules rawRules = {
			MakeRule("S", {{"E", "#"}}),
			MakeRule("E", {{"E", "+", "T"}, {"E", "-", "T"}, {"T"}}),
			MakeRule("T", {{"T", "*", "F"}, {"T", "/", "F"}, {"F"}}),
			MakeRule("F", {{"I"}, {"I", "^", "N"}, {"(", "E", ")"}}),
			MakeRule("I", {{"a"}, {"b"}, {"c"}, {"d"}}),
			MakeRule("N", {{"2"}, {"3"}, {"4"}})};

		PrintRules(rawRules, "Исходная грамматика");
		auto optimizedRules = OptimizeArithmeticGrammar(rawRules, startSymbol);

		GuideSetsCalculator calculator(optimizedRules, startSymbol);
		auto rulesWithGuides = calculator.Calculate();
		LlValidator validator(rulesWithGuides);

		if (validator.IsValid())
		{
			std::cout << "[Result] Грамматика является LL(1)\n";
			TableBuilder builder(rulesWithGuides);
			auto tableRows = builder.BuildDetailed();

			ParseTablePrinter::PrintDetailedRows(tableRows);
			Ll1Parser parser(tableRows, startSymbol);
			std::string input = "a+b*c#";

			std::cout << "\nParsing: " << input << "\n";
			auto tokens = Tokenize(input);
			auto steps = parser.Parse(tokens);

			for (const auto& s : steps)
			{
				std::cout
					<< s.stepNumber << " | "
					<< s.stack << " | "
					<< s.input << " | "
					<< s.action << "\n";
			}
		}
		else
		{
			std::cout << "[Result] Грамматика НЕ является LL(1)\n";
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}