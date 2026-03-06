#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llValidator/LlValidator.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "vm/machine/VirtualMachine.h"
#include "vm/parser/BytecodeParser.h"
#include "vm/value/Value.h"
#include <iostream>
#include <variant>
#include <windows.h>

using namespace PrintGrammar;

int main(int argc, char* argv[])
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