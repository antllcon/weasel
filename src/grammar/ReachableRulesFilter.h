#pragma once
#include "GrammarTypes.h"

class ReachableRulesFilter
{
public:
	explicit ReachableRulesFilter(raw::Rules rules);

	[[nodiscard]] raw::Rules FilterUnreachableRules() const;

private:
	raw::Rules m_rules;
};