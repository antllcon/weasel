#pragma once

#include <string>
#include <vector>

struct ParseTableRow;
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
	Ll1Parser(std::vector<ParseTableRow> tableRows, std::string startSymbol);
	std::vector<ParseStep> Parse(const std::vector<std::string>& tokens);

private:
	std::vector<ParseTableRow> m_rows;
	std::string m_start;
};