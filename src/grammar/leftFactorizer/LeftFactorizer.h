#pragma once
#include "src/grammar/GrammarTypes.h"

class LeftFactorizer
{
public:
	explicit LeftFactorizer(raw::Rules rules);
	[[nodiscard]] raw::Rules Factorize();

private:
	raw::Rules m_rules;
};