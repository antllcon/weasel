#include "GrammarOptimizer.h"
#include "src/grammar/emptyRulesDeleter/EmptyRulesDeleter.h"
#include "src/grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "src/grammar/leftFactorizer/LeftFactorizer.h"
#include "src/grammar/leftRecursionEliminator/LeftRecursionEliminator.h"
#include "src/grammar/llValidator/LlValidator.h"
#include "src/grammar/productiveRulesFilter/ProductiveRulesFilter.h"
#include "src/grammar/reachableRulesFilter/ReachableRulesFilter.h"
#include "src/grammar/unitRulesDeleter/UnitRulesDeleter.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

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

bool HasFlag(OptimizationFlags value, OptimizationFlags flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

bool IsGrammarValidLL1(const raw::Rules& rules, const std::string& startSymbol)
{
	if (rules.empty())
	{
		return false;
	}

	try
	{
		GuideSetsCalculator calculator(rules, startSymbol);
		Rules rulesWithGuides = calculator.Calculate();
		LlValidator validator(rulesWithGuides);
		return validator.IsValid();
	}
	catch (const std::exception&)
	{
		return false;
	}
}

// TODO: мб рандомные штуки
std::vector<OptimizationFlags> GetOptimizationProfiles()
{
	return {
		OptimizationFlags::None,
		OptimizationFlags::LeftFactorize,
		OptimizationFlags::EliminateLeftRecursion,
		OptimizationFlags::EliminateLeftRecursion | OptimizationFlags::LeftFactorize,
		OptimizationFlags::FilterUnreachable | OptimizationFlags::FilterUnproductive | OptimizationFlags::EliminateLeftRecursion | OptimizationFlags::LeftFactorize,
		OptimizationFlags::DeleteUnitRules | OptimizationFlags::FilterUnreachable | OptimizationFlags::FilterUnproductive | OptimizationFlags::EliminateLeftRecursion | OptimizationFlags::LeftFactorize,
		OptimizationFlags::DeleteEmptyRules | OptimizationFlags::DeleteUnitRules | OptimizationFlags::FilterUnreachable | OptimizationFlags::FilterUnproductive | OptimizationFlags::EliminateLeftRecursion | OptimizationFlags::LeftFactorize};
}

std::vector<OptimizationFlags> GenerateAllFlagCombinations()
{
	constexpr OptimizationFlags allFlags[] = {
		OptimizationFlags::DeleteEmptyRules,
		OptimizationFlags::DeleteUnitRules,
		OptimizationFlags::FilterUnproductive,
		OptimizationFlags::FilterUnreachable,
		OptimizationFlags::EliminateLeftRecursion,
		OptimizationFlags::LeftFactorize};

	constexpr size_t flagCount = std::size(allFlags);

	std::vector<OptimizationFlags> combinations;

	constexpr uint32_t total = 1u << flagCount;

	for (uint32_t mask = 0; mask < total; ++mask)
	{
		uint32_t value = 0;

		for (size_t i = 0; i < flagCount; ++i)
		{
			if (mask & 1u << i)
			{
				value |= static_cast<uint32_t>(allFlags[i]);
			}
		}

		combinations.push_back(static_cast<OptimizationFlags>(value));
	}

	return combinations;
}

raw::Rules AugmentGrammar(raw::Rules rules, const std::string& startSymbol, std::string& outNewStart)
{
	outNewStart = "Z";
	while (std::ranges::any_of(rules, [&](const raw::Rule& r) { return r.name == outNewStart; }))
	{
		outNewStart += "'";
	}

	raw::Rule rootRule;
	rootRule.name = outNewStart;
	rootRule.alternatives.push_back({startSymbol, END_SYMBOL});

	rules.insert(rules.begin(), rootRule);
	return rules;
}
} // namespace

namespace GrammarOptimizer
{
raw::Rules Optimize(raw::Rules rules, const std::string& startSymbol, OptimizationFlags flags)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	if (HasFlag(flags, OptimizationFlags::DeleteEmptyRules))
	{
		rules = EmptyRulesDeleter(std::move(rules)).DeleteEmptyRules();
	}

	if (HasFlag(flags, OptimizationFlags::DeleteUnitRules))
	{
		rules = UnitRulesDeleter(std::move(rules)).DeleteUnitRules();
	}

	if (HasFlag(flags, OptimizationFlags::FilterUnproductive))
	{
		rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	}

	if (HasFlag(flags, OptimizationFlags::FilterUnreachable))
	{
		rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	}

	if (HasFlag(flags, OptimizationFlags::EliminateLeftRecursion))
	{
		rules = LeftRecursionEliminator(std::move(rules)).Eliminate();
	}

	if (HasFlag(flags, OptimizationFlags::LeftFactorize))
	{
		rules = LeftFactorizer(std::move(rules)).Factorize();
	}

	if (HasFlag(flags, OptimizationFlags::FilterUnreachable) && (HasFlag(flags, OptimizationFlags::EliminateLeftRecursion) || HasFlag(flags, OptimizationFlags::LeftFactorize)))
	{
		rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();
	}

	return rules;
}

AutoOptimizationResult FindBestLL1(const raw::Rules& rules, const std::string& startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	std::string newStartSymbol;
	raw::Rules augmentedRules = AugmentGrammar(rules, startSymbol, newStartSymbol);

	if (IsGrammarValidLL1(augmentedRules, newStartSymbol))
	{
		return {true, OptimizationFlags::None, augmentedRules, newStartSymbol};
	}

	std::vector<OptimizationFlags> combinations = GenerateAllFlagCombinations();

	for (OptimizationFlags flags : combinations)
	{
		raw::Rules optimized = Optimize(augmentedRules, newStartSymbol, flags);

		if (optimized.empty())
		{
			continue;
		}

		if (IsGrammarValidLL1(optimized, newStartSymbol))
		{
			return {true, flags, std::move(optimized), newStartSymbol};
		}
	}

	return {false, OptimizationFlags::None, augmentedRules, newStartSymbol};
}

raw::Rules OptimizeForLalr(raw::Rules rules, const std::string& startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	rules = ReachableRulesFilter(std::move(rules), startSymbol).FilterUnreachableRules();

	return rules;
}

raw::Rules AugmentGrammarLalr(raw::Rules rules, const std::string& startSymbol, std::string& outNewStart)
{
	outNewStart = "Z";
	while (std::ranges::any_of(rules, [&](const raw::Rule& r) { return r.name == outNewStart; }))
	{
		outNewStart += "'";
	}

	raw::Rule rootRule;
	rootRule.name = outNewStart;
	rootRule.alternatives.push_back({startSymbol});

	rules.insert(rules.begin(), std::move(rootRule));
	return rules;
}
} // namespace GrammarOptimizer