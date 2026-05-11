#include "StringExpr.h"
#include "IAstVisitor.h"

StringExpr::StringExpr(std::string value, const SourceRange& range)
	: m_value(std::move(value))
	, m_range(range)
{
}

void StringExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange StringExpr::GetRange() const
{
	return m_range;
}

const std::string& StringExpr::GetValue() const
{
	return m_value;
}