#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/ClassicForStmt.h"
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/semantic/AstAnnotations.h"
#include "src/compiler/semantic/SymbolTable.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct FunctionInfo
{
	std::shared_ptr<TypeInfo> returnType;
	std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> params;
	uint32_t maxSlots = 0;
};

struct CodegenContext
{
	AstAnnotations annotations;
	std::unordered_map<const AstNode*, SymbolInfo> symbols;
	std::unordered_map<const AstNode*, uint32_t> varDeclSlots;
	std::unordered_map<const AstNode*, std::vector<SymbolInfo>> repIterators;
	std::unordered_map<const ClassicForStmt*, SymbolInfo> classicForInits;
	std::unordered_map<std::string, FunctionInfo> functions;
};