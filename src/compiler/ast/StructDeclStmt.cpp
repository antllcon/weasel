#include "StructDeclStmt.h"
#include "IAstVisitor.h"

StructDeclStmt::StructDeclStmt(
	std::string name, std::vector<FieldDecl> fields, const SourceRange& range)
	: m_name(std::move(name))
	, m_fields(std::move(fields))
	, m_range(range)
{
}

void StructDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange StructDeclStmt::GetRange() const
{
	return m_range;
}

const std::string& StructDeclStmt::GetName() const
{
	return m_name;
}

const std::vector<FieldDecl>& StructDeclStmt::GetFields() const
{
	return m_fields;
}