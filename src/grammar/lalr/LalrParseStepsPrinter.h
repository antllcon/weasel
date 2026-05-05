#pragma once

#include "LalrParser.h"
#include <string>
#include <vector>

class LalrParseStepsPrinter
{
public:
	static void Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine);
};