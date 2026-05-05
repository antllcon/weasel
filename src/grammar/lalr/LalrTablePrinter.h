#pragma once

#include "src/grammar/lalr/LalrTypes.h"

class LalrTablePrinter
{
public:
	static void Print(const LalrTable& table);
};