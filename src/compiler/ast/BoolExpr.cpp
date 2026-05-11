#include "BoolExpr.h"
#include "IAstVisitor.h"

BoolExpr::BoolExpr(bool value, const SourceRange& range)
	: m_value(value)
	, m_range(range)
{
}

void BoolExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange BoolExpr::GetRange() const
{
	return m_range;
}

bool BoolExpr::GetValue() const
{
	return m_value;
}