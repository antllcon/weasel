#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/ast/VarDeclStmt.h"
#include "src/compiler/semantic/AstAnnotations.h"
#include "src/compiler/semantic/SymbolTable.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Expr;

struct ParamInfo
{
	std::string name;
	std::shared_ptr<TypeInfo> type;
	std::optional<VarModifier> modifier;
	const Expr* defaultValue = nullptr;
};

struct FunctionInfo
{
	std::shared_ptr<TypeInfo> returnType;
	std::vector<ParamInfo> params;
	uint32_t maxSlots = 0;
};

struct CollectionLoopInfo
{
	SymbolInfo valueIter;
	uint32_t arraySlot;
	uint32_t counterSlot;
};

struct TimesLoopInfo
{
	uint32_t limitSlot;
	uint32_t counterSlot;
};

struct CodegenContext
{
	AstAnnotations annotations;
	std::unordered_map<const AstNode*, SymbolInfo> symbols;
	std::unordered_map<const AstNode*, uint32_t> varDeclSlots;
	std::unordered_map<const AstNode*, SymbolInfo> repIterators;
	std::unordered_map<const AstNode*, CollectionLoopInfo> collectionLoopInfos;
	std::unordered_map<const AstNode*, TimesLoopInfo> repTimesInfos;
	std::unordered_map<std::string, FunctionInfo> functions;
	std::string entryPointKey;
};