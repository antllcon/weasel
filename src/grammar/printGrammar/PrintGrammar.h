#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/grammarOptimizer/GrammarOptimizer.h"
#include <iostream>

namespace PrintGrammar
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts);
void PrintRules(const raw::Rules& rules, const std::string& title = "");
void PrintRulesWithGuides(const Rules& rules, const std::string& title = "");
void PrintAppliedOptimizations(OptimizationFlags flags);
} // namespace PrintGrammar
