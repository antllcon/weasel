#include "GrammarOptimizer.h"
#include "../emptyRulesDeleter/EmptyRulesDeleter.h"
#include "../productiveRulesFilter/ProductiveRulesFilter.h"
#include "../reachableRulesFilter/ReachableRulesFilter.h"
#include "../unitRulesDeleter/UnitRulesDeleter.h"
#include "src/grammar/leftFactorizer/LeftFactorizer.h"
#include "src/grammar/leftRecursionEliminator/LeftRecursionEliminator.h"
#include "src/grammar/printGrammar/PrintGrammar.h"
#include "src/timer/ScopedTimer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void AssertIsStartSymbolPresent(const raw::Rules& rules, const std::string& startSymbol)
{
	const bool isPresent = std::ranges::any_of(rules, [&startSymbol](const raw::Rule& rule) {
		return rule.name == startSymbol;
	});

	if (!isPresent)
	{
		throw std::runtime_error("Стартовый символ не найден в грамматике");
	}
}

bool AreAlternativesEqual(const raw::Alternatives& lhs, const raw::Alternatives& rhs)
{
	if (lhs.size() != rhs.size())
	{
		return false;
	}

	for (size_t i = 0; i < lhs.size(); ++i)
	{
		if (lhs[i] != rhs[i])
		{
			return false;
		}
	}

	return true;
}

bool AreRulesEqual(const raw::Rules& lhs, const raw::Rules& rhs)
{
	if (lhs.size() != rhs.size())
	{
		return false;
	}

	for (size_t i = 0; i < lhs.size(); ++i)
	{
		if (lhs[i].name != rhs[i].name)
		{
			return false;
		}

		if (!AreAlternativesEqual(lhs[i].alternatives, rhs[i].alternatives))
		{
			return false;
		}
	}

	return true;
}

raw::Rules RunBaseOptimizations(raw::Rules currentRules, const std::string& startSymbol)
{
	currentRules = EmptyRulesDeleter(std::move(currentRules)).DeleteEmptyRules();
	currentRules = UnitRulesDeleter(std::move(currentRules)).DeleteUnitRules();
	currentRules = ProductiveRulesFilter(std::move(currentRules)).FilterUnproductiveRules();
	currentRules = ReachableRulesFilter(std::move(currentRules), startSymbol).FilterUnreachableRules();
	return currentRules;
}
} // namespace

namespace GrammarOptimizer
{
raw::Rules OptimizeForBottomUp(raw::Rules rules, const std::string& startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	bool changed;
	do
	{
		raw::Rules previous = rules;
		rules = RunBaseOptimizations(std::move(rules), startSymbol);
		changed = !AreRulesEqual(rules, previous);
	} while (changed);

	return rules;
}

raw::Rules OptimizeForLL1(raw::Rules rules, const std::string& startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	rules = UnitRulesDeleter(std::move(rules)).DeleteUnitRules();
	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	rules = LeftRecursionEliminator(std::move(rules)).Eliminate();
	rules = LeftFactorizer(std::move(rules)).Factorize();
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();

	return rules;
}

raw::Rules OptimizeForLL1Log(raw::Rules rules, const std::string& startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	std::cout << "LL1 Optimization Steps\n";

	std::cout << "Step 0: UnitRulesDeleter\n";
	PrintGrammar::PrintRules(rules, "Before UnitRulesDeleter:");
	rules = UnitRulesDeleter(std::move(rules)).DeleteUnitRules();
	PrintGrammar::PrintRules(rules, "After UnitRulesDeleter:");

	std::cout << "Step 1: ProductiveRulesFilter\n";
	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	PrintGrammar::PrintRules(rules, "After ProductiveRulesFilter:");

	std::cout << "Step 2: ReachableRulesFilter\n";
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	PrintGrammar::PrintRules(rules, "After ReachableRulesFilter:");

	std::cout << "Step 3: LeftRecursionEliminator\n";
	rules = LeftRecursionEliminator(std::move(rules)).Eliminate();
	PrintGrammar::PrintRules(rules, "After LeftRecursionEliminator:");

	std::cout << "Step 4: LeftFactorizer\n";
	rules = LeftFactorizer(std::move(rules)).Factorize();
	PrintGrammar::PrintRules(rules, "After LeftFactorizer:");

	std::cout << "Step 5: ReachableRulesFilter (cleanup)\n";
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	PrintGrammar::PrintRules(rules, "Final Grammar:");

	return rules;
}
} // namespace GrammarOptimizer