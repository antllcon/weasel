#pragma once
#include "src/grammar/lalrTableBuilder/Lalr1Types.h"

class Lalr1TablePrinter
{
public:
	static void Print(const LalrTable& table);
};