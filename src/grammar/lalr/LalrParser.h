#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/cst/CstInputToken.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrTypes.h"
#include <memory>
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
	[[nodiscard]] std::unique_ptr<CstNode> ParseToTree(const std::vector<CstInputToken>& tokens) const;

private:
	LalrTable m_table;
};