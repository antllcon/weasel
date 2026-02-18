#pragma once
#include "GrammarTypes.h"

class ReachableRulesFilter
{
public:
	explicit ReachableRulesFilter(raw::Rules rules, const std::string& startSymbol);

	[[nodiscard]] raw::Rules FilterUnreachableRules() const;

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};