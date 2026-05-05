#pragma once

#include "src/grammar/lalr/LalrTypes.h"
#include "src/utils/logger/ILogger.h"
#include <memory>

class LalrTablePrinter
{
public:
	static void Print(const LalrTable& table, const std::shared_ptr<ILogger>& logger);
};