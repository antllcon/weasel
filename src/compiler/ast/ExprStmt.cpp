#include "ExprStmt.h"

ExprStmt::ExprStmt(std::unique_ptr<Expr> expr, const SourceRange& range)
	: m_expr(std::move(expr))
	, m_range(range)
{
}

void ExprStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ExprStmt::GetRange() const
{
	return m_range;
}

const Expr& ExprStmt::GetExpr() const
{
	return *m_expr;
}
