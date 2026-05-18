#pragma once

#include "src/compiler/ast/AstNode.h"
#include "src/compiler/sema/SymbolTable.h"
#include <unordered_map>

class SymbolTableVisualizer
{
public:
	static void Visualize(const std::unordered_map<const AstNode*, SymbolInfo>& symbols);
};