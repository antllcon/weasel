#pragma once
#include "src/grammar/GrammarTypes.h"

namespace PrintGrammar
{
raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts);
void PrintRules(const raw::Rules& rules, const std::string& title = "");
void PrintRulesWithGuides(const Rules& rules, const std::string& title = "");

} // namespace PrintGrammar
