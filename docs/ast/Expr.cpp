#include "Expr.h"

void Expr::SetResolvedType(std::shared_ptr<TypeInfo> type)
{
	m_resolvedType = std::move(type);
}

std::shared_ptr<TypeInfo> Expr::GetResolvedType() const
{
	return m_resolvedType;
}

void Expr::MarkPoisoned()
{
	m_isPoisoned = true;
}

bool Expr::IsPoisoned() const
{
	return m_isPoisoned;
}