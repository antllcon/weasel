#include "IdentifierExpr.h"

IdentifierExpr::IdentifierExpr(std::string name, const SourceRange& range)
	: m_name(std::move(name))
	, m_range(range)
{
}

void IdentifierExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange IdentifierExpr::GetRange() const
{
	return m_range;
}

const std::string& IdentifierExpr::GetName() const
{
	return m_name;
}