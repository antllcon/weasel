#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/lalrTableBuilder/Lalr1Types.h"
#include <string>
#include <vector>

struct LalrParseStep
{
	size_t stepNumber;
	std::string stateStack;
	std::string symbolStack;
	std::string input;
	std::string action;
};

class Lalr1Parser
{
public:
	explicit Lalr1Parser(LalrTable table);

	[[nodiscard]] std::vector<LalrParseStep> Parse(const std::vector<std::string>& tokens) const;

private:
	LalrTable m_table;
};