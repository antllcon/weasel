#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/lalrTableBuilder/Lalr1Types.h"
#include <string>

class Lalr1TableBuilder
{
public:
	Lalr1TableBuilder(raw::Rules rules, std::string startSymbol);

	[[nodiscard]] LalrTable Build() const;

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};