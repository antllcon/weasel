#include "FunctionCallExpr.h"

FunctionCallExpr::FunctionCallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args, const SourceRange& range)
	: m_name(std::move(name))
	, m_args(std::move(args))
	, m_range(range)
{
}

void FunctionCallExpr::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange FunctionCallExpr::GetRange() const
{
	return m_range;
}

const std::string& FunctionCallExpr::GetName() const
{
	return m_name;
}

const std::vector<std::unique_ptr<Expr>>& FunctionCallExpr::GetArgs() const
{
	return m_args;
}
