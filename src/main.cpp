#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "timer/ScopedTimer.h"

#include <iostream>
#include <windows.h>

namespace
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
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
} // namespace

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::string startSymbol = "<S>";

		raw::Rules rawRules = {
			MakeRule("<S>", {{"a", "<S>", "b"}, {"c", "<A>", "b"}, {"a", "<D>", "b", "<A>", "c", "<B>", "<C>"}}),
			MakeRule("<A>", {{"<B>", "x"}, {"<A>", "y"}}),
			MakeRule("<C>", {{"<A>", "x"}, {"a"}, {"b"}, {"<B>", "x"}, {"<S>", "y"}, {"<D>"}}),
			MakeRule("<B>", {{"x", "<A>", "y"}, {"<D>", "x"}}),
			MakeRule("<D>", {{"<A>", "<B>"}, {"<D>", "f"}, {"<C>"}})};

		PrintRules(rawRules, "1. Исходная грамматика");

		ScopedTimer timer("оптимизация к LL1", std::cout);
		auto optimRules = GrammarOptimizer::OptimizeForLL1(rawRules, startSymbol);

		PrintRules(optimRules, "2. Оптимизированная грамматика");
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}