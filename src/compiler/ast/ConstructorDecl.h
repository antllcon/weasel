#pragma once
#include "BlockStmt.h"
#include "FunctionDeclStmt.h"
#include "SourceRange.h"
#include <memory>
#include <vector>

struct ConstructorDecl
{
	std::vector<Param> params;
	std::unique_ptr<BlockStmt> body;
	SourceRange range;
};
