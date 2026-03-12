#pragma once
#include "src/grammar/tableBilder/Ll1Parser.h"
#include <string>
#include <vector>

class ParseStepsPrinter
{
public:
	static void Print(const std::vector<ParseStep>& steps, const std::string& inputLine);
};