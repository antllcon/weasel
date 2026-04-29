#pragma once

#include "src/grammar/lalr/LalrTypes.h"
#include <memory>
#include <string>

struct LanguageContext
{
	std::unique_ptr<LalrTable> lalrTable;
	std::string startSymbol;
};