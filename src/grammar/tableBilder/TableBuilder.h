#pragma once

#include "src/grammar/GrammarTypes.h"

#include <string>
#include <unordered_set>
#include <vector>

struct ParseTableRow
{
	enum class Kind
	{
		NonterminalHeader,
		RhsTerminal,
		RhsNonterminal,
		RhsEmpty
	};

	size_t idx;
	Kind kind;
	std::string name;
	std::unordered_set<std::string> guides;
	size_t groupPointer = SIZE_MAX;
	size_t nextPointer = SIZE_MAX;
	size_t ruleIndex = SIZE_MAX;
	size_t altIndex = SIZE_MAX;
};

class TableBuilder
{
public:
	TableBuilder(const Rules& rules);
	std::vector<ParseTableRow> BuildDetailed() const;

private:
	Rules m_rules;
};