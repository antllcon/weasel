#pragma once
#include "Lalr1Parser.h"
#include <string>
#include <vector>

class LalrParseStepsPrinter
{
public:
	static void Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine);
};