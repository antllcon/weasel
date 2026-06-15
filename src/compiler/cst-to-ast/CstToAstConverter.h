#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/grammar/cst/CstNode.h"
#include <memory>
#include <vector>

class CstToAstConverter
{
public:
	[[nodiscard]] static std::unique_ptr<AstNode> Convert(const CstNode& root);
	[[nodiscard]] static std::vector<std::unique_ptr<AstNode>> ConvertDeclarations(const CstNode& root);
};
