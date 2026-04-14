#pragma once

#include "src/grammar/GrammarTypes.h"
#include "src/grammar/grammarOptimizer/GrammarOptimizer.h"
#include "src/logger/ILogger.h"
#include <memory>
#include <string>

namespace PrintGrammar
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts);
void PrintRules(const raw::Rules& rules, const std::shared_ptr<ILogger>& logger, const std::string& title = "");
void PrintRulesWithGuides(const Rules& rules, const std::shared_ptr<ILogger>& logger, const std::string& title = "");
void PrintAppliedOptimizations(OptimizationFlags flags, const std::shared_ptr<ILogger>& logger);
} // namespace PrintGrammar