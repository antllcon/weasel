#include "AssignStmt.h"

AssignStmt::AssignStmt(
	std::unique_ptr<Expr> lhs,
	bool isMove,
	std::unique_ptr<Expr> rhs,
	const SourceRange& range)
	: m_lhs(std::move(lhs))
	, m_isMove(isMove)
	, m_rhs(std::move(rhs))
	, m_range(range)
{
}

void AssignStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange AssignStmt::GetRange() const
{
	return m_range;
}

const Expr& AssignStmt::GetLhs() const
{
	return *m_lhs;
}

const Expr& AssignStmt::GetRhs() const
{
	return *m_rhs;
}

bool AssignStmt::IsMove() const
{
	return m_isMove;
}
