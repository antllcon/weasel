#include "ClassicForStmt.h"

ClassicForStmt::ClassicForStmt(
	std::string initType,
	std::string initName,
	std::unique_ptr<Expr> initExpr,
	std::unique_ptr<Expr> condition,
	std::unique_ptr<Stmt> step,
	std::unique_ptr<Stmt> body,
	const SourceRange& range)
	: m_initType(std::move(initType))
	, m_initName(std::move(initName))
	, m_initExpr(std::move(initExpr))
	, m_condition(std::move(condition))
	, m_step(std::move(step))
	, m_body(std::move(body))
	, m_range(range)
{
}

void ClassicForStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ClassicForStmt::GetRange() const
{
	return m_range;
}

const std::string& ClassicForStmt::GetInitType() const
{
	return m_initType;
}

const std::string& ClassicForStmt::GetInitName() const
{
	return m_initName;
}

const Expr& ClassicForStmt::GetInitExpr() const
{
	return *m_initExpr;
}

const Expr& ClassicForStmt::GetCondition() const
{
	return *m_condition;
}

const Stmt& ClassicForStmt::GetStep() const
{
	return *m_step;
}

const Stmt& ClassicForStmt::GetBody() const
{
	return *m_body;
}
