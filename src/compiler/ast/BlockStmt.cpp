#include "BlockStmt.h"

BlockStmt::BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, const SourceRange& range)
	: m_stmts(std::move(stmts))
	, m_range(range)
{
}

void BlockStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange BlockStmt::GetRange() const
{
	return m_range;
}

const std::vector<std::unique_ptr<Stmt>>& BlockStmt::GetStmts() const
{
	return m_stmts;
}
