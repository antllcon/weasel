#pragma once
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/llTableBuilder/Ll1TableTypes.h"

class Ll1TableBuilder
{
public:
	explicit Ll1TableBuilder(Rules rules);
	[[nodiscard]] Ll1Table Build() const;

private:
	Rules m_rules;
};