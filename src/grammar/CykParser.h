#pragma once
#include "GrammarTypes.h"
#include <string>
#include <vector>

class CykParser
{
public:
	explicit CykParser(raw::Rules rules, std::string startSymbol);

	[[nodiscard]] CykParseResult Parse(const std::string& word) const;

private:
	raw::Rules m_rules;
	std::string m_startSymbol;
};