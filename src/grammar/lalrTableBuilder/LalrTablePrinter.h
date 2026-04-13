#pragma once
#include "src/grammar/lalrTableBuilder/LalrTypes.h"

class LalrTablePrinter
{
public:
	static void Print(const LalrTable& table);
};