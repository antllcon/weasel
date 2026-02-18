#pragma once
#include "GrammarTypes.h"

class LongRulesSplitter
{
public:
	explicit LongRulesSplitter(raw::Rules rules);

	[[nodiscard]] raw::Rules SplitLongRules();

private:
	raw::Rules m_rules;
	int m_nextId;
};