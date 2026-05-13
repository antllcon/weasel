#pragma once

#include "src/grammar/lalr/LalrTypes.h"
#include <string>
#include <vector>

namespace LalrParseStepsPrinter
{
void Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine);
}