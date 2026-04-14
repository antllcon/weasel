#pragma once
#include "src/grammar/GrammarTypes.h"
#include <string>

class ReachableRulesFilter
{
public:
	ReachableRulesFilter(raw::Rules rules, std::string_view startSymbol);
	[[nodiscard]] raw::Rules FilterUnreachableRules();

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};