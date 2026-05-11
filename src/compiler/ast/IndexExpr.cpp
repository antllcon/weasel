#include "IndexExpr.h"
#include "IAstVisitor.h"

IndexExpr::IndexExpr(
	std::unique_ptr<Expr> receiver, std::unique_ptr<Expr> index, const SourceRange& range)
	: m_receiver(std::move(receiver))
	, m_index(std::move(index))
	, m_range(range)
{
}

void IndexExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange IndexExpr::GetRange() const
{
	return m_range;
}

const Expr& IndexExpr::GetReceiver() const
{
	return *m_receiver;
}

const Expr& IndexExpr::GetIndex() const
{
	return *m_index;
}