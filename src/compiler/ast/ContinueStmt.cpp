#include "ContinueStmt.h"

ContinueStmt::ContinueStmt(const SourceRange& range)
	: m_range(range)
{
}

void ContinueStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ContinueStmt::GetRange() const
{
	return m_range;
}
