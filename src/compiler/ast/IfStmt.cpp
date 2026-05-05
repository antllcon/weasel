#include "IfStmt.h"

IfStmt::IfStmt(
	std::unique_ptr<Expr> condition,
	std::unique_ptr<BlockStmt> thenBlock,
	std::unique_ptr<Stmt> elseNode,
	const SourceRange& range)
	: m_condition(std::move(condition))
	, m_thenBlock(std::move(thenBlock))
	, m_elseNode(std::move(elseNode))
	, m_range(range)
{
}

void IfStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange IfStmt::GetRange() const
{
	return m_range;
}

const Expr& IfStmt::GetCondition() const
{
	return *m_condition;
}

const BlockStmt& IfStmt::GetThenBlock() const
{
	return *m_thenBlock;
}

const Stmt* IfStmt::GetElseNode() const
{
	return m_elseNode.get();
}
