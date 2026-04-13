#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/lalrTableBuilder/LalrTypes.h"
#include <string>

class LalrTableBuilder
{
public:
	LalrTableBuilder(raw::Rules rules, std::string startSymbol);

	[[nodiscard]] LalrTable Build() const;

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};