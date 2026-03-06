#pragma once
#include "src/grammar/GrammarTypes.h"

namespace PrintGrammar
{
inline raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts);
inline void PrintRules(const raw::Rules& rules, const std::string& title = "");
inline void PrintRulesWithGuides(const Rules& rules, const std::string& title = "");

} // namespace PrintGrammar
