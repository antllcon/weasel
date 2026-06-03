#pragma once
#include "Expr.h"
#include "Stmt.h"
#include <memory>
#include <vector>

struct WhenEntry
{
	std::vector<std::unique_ptr<Expr>> conditions;
	std::unique_ptr<Stmt> body;
};