#pragma once
#include "src/grammar/llTableBuilder/Ll1TableTypes.h"
#include <string>
#include <vector>

struct ParseStep
{
	size_t stepNumber;
	std::string stack;
	std::string input;
	std::string action;
};

class Ll1Parser
{
public:
	Ll1Parser(Ll1Table table, std::string startSymbol);
	[[nodiscard]] std::vector<ParseStep> Parse(const std::vector<std::string>& tokens) const;

private:
	Ll1Table m_table;
	std::string m_start;
};