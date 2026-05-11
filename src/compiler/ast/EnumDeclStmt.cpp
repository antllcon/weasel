#include "EnumDeclStmt.h"
#include "IAstVisitor.h"

EnumDeclStmt::EnumDeclStmt(
	std::string name, std::vector<std::string> values, const SourceRange& range)
	: m_name(std::move(name))
	, m_values(std::move(values))
	, m_range(range)
{
}

void EnumDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange EnumDeclStmt::GetRange() const
{
	return m_range;
}

const std::string& EnumDeclStmt::GetName() const
{
	return m_name;
}

const std::vector<std::string>& EnumDeclStmt::GetValues() const
{
	return m_values;
}