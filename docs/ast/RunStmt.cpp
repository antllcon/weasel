#include "RunStmt.h"

RunStmt::RunStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, const SourceRange& range)
	: m_condition(std::move(condition))
	, m_body(std::move(body))
	, m_range(range)
{
}

void RunStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange RunStmt::GetRange() const
{
	return m_range;
}

const Expr& RunStmt::GetCondition() const
{
	return *m_condition;
}

const Stmt& RunStmt::GetBody() const
{
	return *m_body;
}