#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/ast/TypeInfo.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

struct AstAnnotations
{
	std::unordered_map<const AstNode*, std::shared_ptr<TypeInfo>> resolvedTypes;
	std::unordered_set<const AstNode*> poisonedNodes;
};