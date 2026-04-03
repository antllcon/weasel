#include "ImplicitCastExpr.h"

ImplicitCastExpr::ImplicitCastExpr(std::unique_ptr<Expr> source, CastKind kind)
	: m_source(std::move(source))
	, m_kind(kind)
{
}

void ImplicitCastExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ImplicitCastExpr::GetRange() const
{
	return m_source->GetRange();
}

const Expr& ImplicitCastExpr::GetSource() const
{
	return *m_source;
}

CastKind ImplicitCastExpr::GetCastKind() const
{
	return m_kind;
}