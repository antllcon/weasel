#include "RepTimesStmt.h"

RepTimesStmt::RepTimesStmt(std::unique_ptr<Expr> countExpr,
	std::unique_ptr<Stmt> body,
	const SourceRange& range)
	: m_countExpr(std::move(countExpr))
	, m_body(std::move(body))
	, m_range(range)
{
}

void RepTimesStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange RepTimesStmt::GetRange() const
{
	return m_range;
}

const Expr& RepTimesStmt::GetCountExpr() const
{
	return *m_countExpr;
}

const Stmt& RepTimesStmt::GetBody() const
{
	return *m_body;
}
