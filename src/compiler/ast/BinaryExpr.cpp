#include "BinaryExpr.h"

BinaryExpr::BinaryExpr(BinaryOpKind op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
	: m_op(op)
	, m_left(std::move(left))
	, m_right(std::move(right))
{
	if (m_left->IsPoisoned() || m_right->IsPoisoned())
	{
		MarkPoisoned();
	}
}

void BinaryExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange BinaryExpr::GetRange() const
{
	return SourceRange{m_left->GetRange().start, m_right->GetRange().end};
}

BinaryOpKind BinaryExpr::GetOp() const
{
	return m_op;
}
const Expr& BinaryExpr::GetLeft() const
{
	return *m_left;
}
const Expr& BinaryExpr::GetRight() const
{
	return *m_right;
}