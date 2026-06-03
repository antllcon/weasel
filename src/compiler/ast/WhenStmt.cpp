#include "WhenStmt.h"
#include "IAstVisitor.h"

WhenStmt::WhenStmt(
	std::unique_ptr<Expr> subject,
	std::vector<WhenEntry> entries,
	std::unique_ptr<Stmt> elseBody,
	const SourceRange& range)
	: m_subject(std::move(subject))
	, m_entries(std::move(entries))
	, m_elseBody(std::move(elseBody))
	, m_range(range)
{
}

void WhenStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange WhenStmt::GetRange() const
{
	return m_range;
}

const Expr* WhenStmt::GetSubject() const
{
	return m_subject.get();
}

const std::vector<WhenEntry>& WhenStmt::GetEntries() const
{
	return m_entries;
}

const Stmt* WhenStmt::GetElseBody() const
{
	return m_elseBody.get();
}