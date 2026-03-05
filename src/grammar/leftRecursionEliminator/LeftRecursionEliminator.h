#pragma once
#include "src/grammar/GrammarTypes.h"

class LeftRecursionEliminator
{
public:
	explicit LeftRecursionEliminator(raw::Rules rules);
	[[nodiscard]] raw::Rules Eliminate();

private:
	raw::Rules m_rules;
};