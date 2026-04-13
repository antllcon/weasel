#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/lalrTableBuilder/LalrTypes.h"
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

class LalrParser
{
public:
	explicit LalrParser(LalrTable table);

	[[nodiscard]] std::vector<LalrParseStep> Parse(const std::vector<std::string>& tokens) const;

private:
	LalrTable m_table;
};