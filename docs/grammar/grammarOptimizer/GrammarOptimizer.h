#pragma once
#include "../GrammarTypes.h"
#include <cstdint>
#include <string>

enum class OptimizationFlags : uint32_t
{
	None = 0,
	DeleteEmptyRules = 1 << 0,
	DeleteUnitRules = 1 << 1,
	FilterUnproductive = 1 << 2,
	FilterUnreachable = 1 << 3,
	EliminateLeftRecursion = 1 << 4,
	LeftFactorize = 1 << 5
};

inline OptimizationFlags operator|(OptimizationFlags lhs, OptimizationFlags rhs)
{
	return static_cast<OptimizationFlags>(
		static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

namespace GrammarOptimizer
{
struct AutoOptimizationResult
{
	bool isFound;
	OptimizationFlags flags;
	raw::Rules rules;
	std::string startSymbol;
};

[[nodiscard]] raw::Rules Optimize(
	raw::Rules rules,
	const std::string& startSymbol,
	OptimizationFlags flags);

[[nodiscard]] AutoOptimizationResult FindBestLL1(
	const raw::Rules& rules,
	const std::string& startSymbol);
} // namespace GrammarOptimizer