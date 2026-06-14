#include "ClassDeclStmt.h"

ClassDeclStmt::ClassDeclStmt(std::string name,
	std::vector<ClassField> fields,
	std::vector<ConstructorDecl> constructors,
	std::vector<ClassMethod> methods,
	const SourceRange& range)
	: m_name(std::move(name))
	, m_fields(std::move(fields))
	, m_constructors(std::move(constructors))
	, m_methods(std::move(methods))
	, m_range(range)
{
}

void ClassDeclStmt::Accept(IAstVisitor& visitor) const
{
	visitor.Visit(*this);
}

SourceRange ClassDeclStmt::GetRange() const
{
	return m_range;
}

const std::string& ClassDeclStmt::GetName() const
{
	return m_name;
}

const std::vector<ClassField>& ClassDeclStmt::GetFields() const
{
	return m_fields;
}

const std::vector<ConstructorDecl>& ClassDeclStmt::GetConstructors() const
{
	return m_constructors;
}

const std::vector<ClassMethod>& ClassDeclStmt::GetMethods() const
{
	return m_methods;
}
