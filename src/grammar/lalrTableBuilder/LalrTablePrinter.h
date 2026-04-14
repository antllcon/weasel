#pragma once

#include "src/grammar/lalrTableBuilder/LalrTypes.h"
#include "src/logger/ILogger.h"
#include <memory>

class LalrTablePrinter
{
public:
	static void Print(const LalrTable& table, const std::shared_ptr<ILogger>& logger);
};