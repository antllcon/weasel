#include "grammar/EmptyRulesDeleter.h"
#include "grammar/GrammarTypes.h"
#include "grammar/ProductiveRulesFilter.h"
#include "grammar/ReachableRulesFilter.h"
#include "timer/Timer.h"
#include <iomanip>
#include <iostream>
#include <windows.h>

namespace
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
}

void PrintRules(const raw::Rules& rules, const std::string& stageName)
{
	std::cout << stageName << std::endl;
	for (const auto& rule : rules)
	{
		std::cout << rule.name << " -> ";
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			for (const auto& symbol : rule.alternatives[i])
			{
				std::cout << symbol << " ";
			}
			if (i + 1 < rule.alternatives.size())
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

		const Timer timer;
		std::cout << std::fixed << std::setprecision(4);
		std::cout << "Пример обработки правил грамматики" << std::endl;

		raw::Rules grammar = {
			MakeRule("<S>", {{"<A>", "b"}, {"<C>"}}),
			MakeRule("<A>", {{"a"}, {EMPTY_SYMBOL}}),
			MakeRule("<C>", {{"<C>", "c"}}),
			MakeRule("<D>", {{"d"}})};

		PrintRules(grammar, "Исходная грамматика");

		EmptyRulesDeleter emptyDeleter(std::move(grammar));
		raw::Rules noEmptyRules = emptyDeleter.DeleteEmptyRules();
		PrintRules(noEmptyRules, "1. После удаления epsilon-правил");

		ProductiveRulesFilter productiveFilter(std::move(noEmptyRules));
		raw::Rules productiveRules = productiveFilter.FilterUnproductiveRules();
		PrintRules(productiveRules, "2. После удаления непродуктивных правил");

		ReachableRulesFilter reachableFilter(std::move(productiveRules));
		raw::Rules finalRules = reachableFilter.FilterUnreachableRules();
		PrintRules(finalRules, "3. После удаления недостижимых правил");

		std::cout << "Время работы: " << timer.Elapsed() << "s" << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[X] Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}