#include "ReturnStmt.h"

ReturnStmt::ReturnStmt(std::unique_ptr<Expr> value, const SourceRange& range)
	: m_value(std::move(value))
	, m_range(range)
{
}

void ReturnStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ReturnStmt::GetRange() const
{
	return m_range;
}

const Expr* ReturnStmt::GetValue() const
{
	return m_value.get();
}
