#include "UnaryExpr.h"
#include "IAstVisitor.h"

UnaryExpr::UnaryExpr(UnaryOpKind op, std::unique_ptr<Expr> operand, const SourceRange& range)
	: m_op(op)
	, m_operand(std::move(operand))
	, m_range(range)
{
}

void UnaryExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange UnaryExpr::GetRange() const
{
	return m_range;
}

UnaryOpKind UnaryExpr::GetOp() const
{
	return m_op;
}

const Expr& UnaryExpr::GetOperand() const
{
	return *m_operand;
}