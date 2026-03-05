#pragma once
#include "src/grammar/GrammarTypes.h"

class LlValidator
{
public:
	explicit LlValidator(Rules rules);
	[[nodiscard]] bool IsValid() const;

private:
	Rules m_rules;
};