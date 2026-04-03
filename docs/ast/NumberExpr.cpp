#include "NumberExpr.h"

NumberExpr::NumberExpr(std::string value, bool isFloat, const SourceRange& range)
	: m_value(std::move(value))
	, m_isFloat(isFloat)
	, m_range(range)
{
}

void NumberExpr::Accept(IAstVisitor& /* visitor */) const
{
	// Нужно будет добавить Visit(const NumberExpr& node) в IAstVisitor.h !
	// visitor.Visit(*this);
}

SourceRange NumberExpr::GetRange() const
{
	return m_range;
}
const std::string& NumberExpr::GetValue() const
{
	return m_value;
}
bool NumberExpr::IsFloat() const
{
	return m_isFloat;
}