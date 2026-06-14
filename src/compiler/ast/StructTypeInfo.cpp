#include "StructTypeInfo.h"

StructTypeInfo::StructTypeInfo(std::string name, std::vector<FieldDecl> fields, bool isClass)
	: m_name(std::move(name))
	, m_fields(std::move(fields))
	, m_isClass(isClass)
{
}

std::string StructTypeInfo::GetName() const
{
	return m_name;
}

bool StructTypeInfo::IsScalar() const
{
	return false;
}

const std::vector<FieldDecl>& StructTypeInfo::GetFields() const
{
	return m_fields;
}

uint32_t StructTypeInfo::GetFieldCount() const
{
	return static_cast<uint32_t>(m_fields.size());
}

bool StructTypeInfo::IsClass() const
{
	return m_isClass;
}
