#include "EnumTypeInfo.h"

EnumTypeInfo::EnumTypeInfo(std::string name, std::vector<std::string> fields)
	: m_name(std::move(name))
	, m_fields(std::move(fields))
{
}

std::string EnumTypeInfo::GetName() const
{
	return m_name;
}

bool EnumTypeInfo::IsScalar() const
{
	return true;
}

const std::vector<std::string>& EnumTypeInfo::GetFields() const
{
	return m_fields;
}