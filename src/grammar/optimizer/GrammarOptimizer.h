#pragma once
#include "../GrammarTypes.h"
#include <string>

namespace GrammarOptimizer
{
[[nodiscard]] raw::Rules OptimizeForLalr(raw::Rules rules, std::string_view startSymbol);
[[nodiscard]] raw::Rules AugmentGrammarLalr(raw::Rules rules, std::string_view startSymbol, std::string& outNewStart);
} // namespace GrammarOptimizer
