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

	const std::vector<OptimizationFlags> profiles = GetOptimizationProfiles();

	for (const auto flags : profiles)
	{
		raw::Rules optimized = Optimize(rules, startSymbol, flags);
		if (IsGrammarValidLL1(optimized, startSymbol))
		{
			return {true, flags, std::move(optimized)};
		}
	}

	return {false, OptimizationFlags::None, {}};
}
} // namespace GrammarOptimizer