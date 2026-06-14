#include "RepCollectionStmt.h"

RepCollectionStmt::RepCollectionStmt(std::string valueIterator,
	std::string indexIterator,
	std::unique_ptr<Expr> collectionExpr,
	std::unique_ptr<Stmt> body,
	const SourceRange& range)
	: m_valueIterator(std::move(valueIterator))
	, m_indexIterator(std::move(indexIterator))
	, m_collectionExpr(std::move(collectionExpr))
	, m_body(std::move(body))
	, m_range(range)
{
}

void RepCollectionStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange RepCollectionStmt::GetRange() const
{
	return m_range;
}

const std::string& RepCollectionStmt::GetValueIterator() const
{
	return m_valueIterator;
}

const std::string& RepCollectionStmt::GetIndexIterator() const
{
	return m_indexIterator;
}

bool RepCollectionStmt::HasIndexIterator() const
{
	return !m_indexIterator.empty();
}

const Expr& RepCollectionStmt::GetCollectionExpr() const
{
	return *m_collectionExpr;
}

const Stmt& RepCollectionStmt::GetBody() const
{
	return *m_body;
}
