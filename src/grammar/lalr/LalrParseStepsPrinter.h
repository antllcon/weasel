#pragma once

#include "LalrParser.h"
#include "src/utils/logger/ILogger.h"
#include <memory>
#include <string>
#include <vector>

class LalrParseStepsPrinter
{
public:
	static void Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine, const std::shared_ptr<ILogger>& logger);
};