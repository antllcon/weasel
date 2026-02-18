#pragma once
#include "GrammarTypes.h"
#include <string>
#include <unordered_map>

class TerminalsIsolator
{
public:
	explicit TerminalsIsolator(raw::Rules rules);

	[[nodiscard]] raw::Rules IsolateTerminals();

private:
	raw::Rules m_rules;
	std::unordered_map<std::string, std::string> m_terminalCache;
};