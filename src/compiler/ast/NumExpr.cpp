#include "NumExpr.h"

NumExpr::NumExpr(std::string value, bool isFloat, const SourceRange& range)
	: m_value(std::move(value))
	, m_isFloat(isFloat)
	, m_range(range)
{
}

void NumExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange NumExpr::GetRange() const
{
	return m_range;
}
const std::string& NumExpr::GetValue() const
{
	return m_value;
}
bool NumExpr::IsFloat() const
{
	return m_isFloat;
}