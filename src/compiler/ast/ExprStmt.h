#pragma once
#include "Stmt.h"
#include "Expr.h"
#include "SourceRange.h"
#include <memory>

class ExprStmt final : public Stmt
{
public:
	ExprStmt(std::unique_ptr<Expr> expr, const SourceRange& range);

	void Accept(IAstVisitor& visitor) const override;
	SourceRange GetRange() const override;

	const Expr& GetExpr() const;

private:
	std::unique_ptr<Expr> m_expr;
	SourceRange m_range;
};
