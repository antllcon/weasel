#include "UnionDeclStmt.h"
#include "IAstVisitor.h"

UnionDeclStmt::UnionDeclStmt(
	std::string name, std::vector<FieldDecl> fields, const SourceRange& range)
	: m_name(std::move(name))
	, m_fields(std::move(fields))
	, m_range(range)
{
}

void UnionDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange UnionDeclStmt::GetRange() const
{
	return m_range;
}

const std::string& UnionDeclStmt::GetName() const
{
	return m_name;
}

const std::vector<FieldDecl>& UnionDeclStmt::GetFields() const
{
	return m_fields;
}