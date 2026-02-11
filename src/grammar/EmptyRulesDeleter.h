#pragma once
#include "GrammarTypes.h"

class EmptyRulesDeleter
{
public:
	explicit EmptyRulesDeleter(raw::Rules rules);

	[[nodiscard]] raw::Rules DeleteEmptyRules();

private:
	raw::Rules m_rules;
};