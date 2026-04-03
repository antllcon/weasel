#include "ErrorExpr.h"

ErrorExpr::ErrorExpr(const SourceRange& range)
	: m_range(range)
{
	MarkPoisoned();
}

void ErrorExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ErrorExpr::GetRange() const
{
	return m_range;
}