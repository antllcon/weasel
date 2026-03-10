#pragma once
#include "../GrammarTypes.h"
#include <string>

namespace GrammarOptimizer
{
[[nodiscard]] raw::Rules OptimizeForBottomUp(raw::Rules rules, const std::string& startSymbol);
[[nodiscard]] raw::Rules OptimizeForLL1(raw::Rules rules, const std::string& startSymbol);
[[nodiscard]] raw::Rules OptimizeForLL1Log(raw::Rules rules, const std::string& startSymbol);
}