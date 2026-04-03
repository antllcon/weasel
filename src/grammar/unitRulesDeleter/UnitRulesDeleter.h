#pragma once
#include "src/grammar/GrammarTypes.h"

class UnitRulesDeleter
{
public:
	explicit UnitRulesDeleter(raw::Rules rules);
	[[nodiscard]] raw::Rules DeleteUnitRules();

private:
	raw::Rules m_rules;
};