#include "RepStmt.h"

RepStmt::RepStmt(std::string iterator,
	std::unique_ptr<Expr> startExpr,
	std::unique_ptr<Expr> endExpr,
	std::unique_ptr<Expr> stepExpr,
	bool isDown,
	std::unique_ptr<Stmt> body,
	const SourceRange& range)
	: m_iterator(std::move(iterator))
	, m_startExpr(std::move(startExpr))
	, m_endExpr(std::move(endExpr))
	, m_stepExpr(std::move(stepExpr))
	, m_isDown(isDown)
	, m_body(std::move(body))
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

const std::string& RepStmt::GetIterator() const
{
	return m_iterator;
}

const Expr& RepStmt::GetStartExpr() const
{
	return *m_startExpr;
}

const Expr& RepStmt::GetEndExpr() const
{
	return *m_endExpr;
}

const Expr* RepStmt::GetStepExpr() const
{
	return m_stepExpr.get();
}

bool RepStmt::IsDown() const
{
	return m_isDown;
}

const Stmt& RepStmt::GetBody() const
{
	return *m_body;
}
