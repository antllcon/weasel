#include "ArrayLiteralExpr.h"
#include "IAstVisitor.h"

ArrayLiteralExpr::ArrayLiteralExpr(
	std::vector<std::unique_ptr<Expr>> elements, const SourceRange& range)
	: m_elements(std::move(elements))
	, m_range(range)
{
}

void ArrayLiteralExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ArrayLiteralExpr::GetRange() const
{
	return m_range;
}

const std::vector<std::unique_ptr<Expr>>& ArrayLiteralExpr::GetElements() const
{
	return m_elements;
}