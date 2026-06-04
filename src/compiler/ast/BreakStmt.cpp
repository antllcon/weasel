#include "BreakStmt.h"

BreakStmt::BreakStmt(const SourceRange& range)
	: m_range(range)
{
}

void BreakStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange BreakStmt::GetRange() const
{
	return m_range;
}
