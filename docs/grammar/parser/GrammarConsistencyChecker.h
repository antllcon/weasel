#pragma once
#include "src/grammar/GrammarTypes.h"

class GrammarConsistencyChecker
{
public:
	static void Check(const raw::Rules& rules);
};