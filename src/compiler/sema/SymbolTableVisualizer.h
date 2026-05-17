#pragma once

#include "src/compiler/sema/SymbolTable.h"
#include <string>
#include <unordered_map>

class SymbolTableVisualizer
{
public:
	static void Visualize(const std::unordered_map<std::string, SymbolInfo>& symbols);
};