#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/grammar/cst/CstNode.h"
#include <memory>

class CstToAstConverter
{
public:
	[[nodiscard]] static std::unique_ptr<AstNode> Convert(const CstNode& root);
};
