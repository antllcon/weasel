#pragma once
#include "GrammarTypes.h"

class ProductiveRulesFilter
{
public:
	explicit ProductiveRulesFilter(raw::Rules rules);

	[[nodiscard]] raw::Rules FilterUnproductiveRules();

private:
	raw::Rules m_rules;
};