#include "GrammarOptimizer.h"
#include "../emptyRulesDeleter/EmptyRulesDeleter.h"
#include "../productiveRulesFilter/ProductiveRulesFilter.h"
#include "../reachableRulesFilter/ReachableRulesFilter.h"
#include "../unitRulesDeleter/UnitRulesDeleter.h"
#include <algorithm>
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

	bool changed;
	do
	{
		raw::Rules previous = rules;
		// rules = LeftRecursionEliminator(std::move(rules)).Eliminate();
		// rules = LeftFactorizer(std::move(rules)).Factorize();
		changed = !AreRulesEqual(rules, previous);
	} while (changed);

	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();

	return rules;
}
} // namespace GrammarOptimizer