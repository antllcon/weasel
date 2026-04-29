#include "RepStmt.h"

RepStmt::RepStmt(std::vector<std::string> iterators,
	std::vector<std::unique_ptr<Expr>> ranges,
	std::unique_ptr<Stmt> originalBody,
	const SourceRange& range)
	: m_iterators(std::move(iterators))
	, m_ranges(std::move(ranges))
	, m_originalBody(std::move(originalBody))
	, m_range(range)
{
}

void RepStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange RepStmt::GetRange() const
{
	return m_range;
}

void RepStmt::SetLoweredBody(std::unique_ptr<Stmt> lowered)
{
	m_loweredBody = std::move(lowered);
}

Stmt* RepStmt::GetLoweredBody() const
{
	return m_loweredBody.get();
}