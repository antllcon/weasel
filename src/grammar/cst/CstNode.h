#pragma once
#include "src/compiler/ast/SourceRange.h"
#include <memory>
#include <string>
#include <vector>

struct CstInputToken
{
	std::string symbol;
	std::string value;
	SourceLocation location;
};

struct CstNode
{
	std::string label;
	std::string value;
	SourceLocation location;
	std::vector<std::unique_ptr<CstNode>> children;
};
