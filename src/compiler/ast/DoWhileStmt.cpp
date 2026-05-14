#include "DoWhileStmt.h"
#include "src/compiler/ast/IAstVisitor.h"

DoWhileStmt::DoWhileStmt(std::unique_ptr<Stmt> body, std::unique_ptr<Expr> condition, const SourceRange& range)
	: m_body(std::move(body))
	, m_condition(std::move(condition))
	, m_range(range)
{
}

void DoWhileStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange DoWhileStmt::GetRange() const
{
	return m_range;
}

const Stmt& DoWhileStmt::GetBody() const
{
	return *m_body;
}

const Expr& DoWhileStmt::GetCondition() const
{
	return *m_condition;
}
