#include "grammar/GrammarTypes.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llValidator/LlValidator.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "grammar/leftFactorizer/LeftFactorizer.h"
#include "grammar/leftRecursionEliminator/LeftRecursionEliminator.h"
#include "grammar/productiveRulesFilter/ProductiveRulesFilter.h"
#include "grammar/reachableRulesFilter/ReachableRulesFilter.h"
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
            MakeRule("N", {{"2"}, {"3"}, {"4"}})
        };

        PrintRules(rawRules, "Исходная грамматика");

        auto optimizedRules = OptimizeArithmeticGrammar(rawRules, startSymbol);

        GuideSetsCalculator calculator(optimizedRules, startSymbol);
    	auto rulesWithGuides = calculator.Calculate();
        LlValidator validator(rulesWithGuides);

        if (validator.IsValid())
        {
            std::cout << "[Result] Грамматика является LL(1)" << std::endl;
        }
        else
        {
            std::cout << "[Result] Грамматика НЕ является LL(1)" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Error] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}