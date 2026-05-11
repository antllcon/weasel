#include "MemberAccessExpr.h"
#include "IAstVisitor.h"

MemberAccessExpr::MemberAccessExpr(
	std::unique_ptr<Expr> receiver, std::string field, const SourceRange& range)
	: m_receiver(std::move(receiver))
	, m_field(std::move(field))
	, m_range(range)
{
}

void MemberAccessExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange MemberAccessExpr::GetRange() const
{
	return m_range;
}

const Expr& MemberAccessExpr::GetReceiver() const
{
	return *m_receiver;
}

const std::string& MemberAccessExpr::GetField() const
{
	return m_field;
}