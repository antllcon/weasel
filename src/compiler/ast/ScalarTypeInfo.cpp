#include "TypeInfo.h"
#include <stdexcept>

namespace
{
std::string FormatName(BaseType base)
{
	switch (base)
	{
	case BaseType::Int:
		return std::string(TypeKeyword::Int);
	case BaseType::Uint:
		return std::string(TypeKeyword::Uint);
	case BaseType::Real:
		return std::string(TypeKeyword::Real);
	case BaseType::Bool:
		return std::string(TypeKeyword::Bool);
	case BaseType::String:
		return std::string(TypeKeyword::String);
	case BaseType::Void:
		return std::string(TypeKeyword::Void);
	default:
		return "custom";
	}
}

BaseType ParseBaseType(const std::string& name)
{
	if (name == TypeKeyword::Int) return BaseType::Int;
	if (name == TypeKeyword::Uint) return BaseType::Uint;
	if (name == TypeKeyword::Real) return BaseType::Real;
	if (name == TypeKeyword::Bool) return BaseType::Bool;
	if (name == TypeKeyword::String) return BaseType::String;
	if (name == TypeKeyword::Void) return BaseType::Void;
	throw std::runtime_error("Неизвестное имя типа: " + name);
}
} // namespace

ScalarTypeInfo::ScalarTypeInfo(BaseType base)
	: m_base(base)
{
}

std::string ScalarTypeInfo::GetName() const
{
	return FormatName(m_base);
}

bool ScalarTypeInfo::IsScalar() const
{
	return true;
}

BaseType ScalarTypeInfo::GetBaseType() const
{
	return m_base;
}

bool ScalarTypeInfo::IsInteger() const
{
	return m_base == BaseType::Int || m_base == BaseType::Uint;
}

bool ScalarTypeInfo::IsFloat() const
{
	return m_base == BaseType::Real;
}

bool ScalarTypeInfo::IsVoid() const
{
	return m_base == BaseType::Void;
}

std::shared_ptr<ScalarTypeInfo> ScalarTypeInfo::Make(BaseType base)
{
	return std::make_shared<ScalarTypeInfo>(base);
}

std::shared_ptr<ScalarTypeInfo> ScalarTypeInfo::FromString(const std::string& typeName)
{
	return std::make_shared<ScalarTypeInfo>(ParseBaseType(typeName));
}