#pragma once
#include "Expr.h"
#include "VarDeclStmt.h"
#include <memory>
#include <optional>
#include <string>

struct ClassField
{
	std::optional<VarModifier> modifier;
	std::string typeName;
	std::string name;
	std::unique_ptr<Expr> defaultValue;
	bool isPublic = true;
};
