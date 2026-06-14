#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/TypeInfo.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct AstAnnotations
{
	std::unordered_map<const AstNode*, std::shared_ptr<TypeInfo>> resolvedTypes;
	std::unordered_set<const AstNode*> poisonedNodes;
	std::unordered_map<const AstNode*, std::string> functionDeclKeys;
	std::unordered_map<const AstNode*, std::string> resolvedCallTargets;
	std::unordered_map<const AstNode*, std::string> ctorKeys;
	std::unordered_map<const AstNode*, uint32_t> fieldAccessIndices;
	std::unordered_set<const AstNode*> implicitThisCalls;
};