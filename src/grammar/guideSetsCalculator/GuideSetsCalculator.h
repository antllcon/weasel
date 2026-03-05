#pragma once
#include "src/grammar/GrammarTypes.h"
#include <string>

class GuideSetsCalculator
{
public:
	GuideSetsCalculator(raw::Rules rules, std::string startSymbol);
	[[nodiscard]] Rules Calculate() const;

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};