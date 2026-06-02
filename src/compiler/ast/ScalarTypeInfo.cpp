#include "TypeInfo.h"
#include "src/compiler/core/LanguageTokens.h"

#include <stdexcept>

namespace
{
std::string FormatName(BaseType base)
{
	switch (base)
	{
	case BaseType::Int:
		return std::string(LanguageTokens::TypeInt);
	case BaseType::Uint:
		return std::string(LanguageTokens::TypeUint);
	case BaseType::Real:
		return std::string(LanguageTokens::TypeReal);
	case BaseType::Bool:
		return std::string(LanguageTokens::TypeBool);
	case BaseType::String:
		return std::string(LanguageTokens::TypeString);
	case BaseType::Void:
		return std::string(LanguageTokens::TypeVoid);
	default:
		return "custom";
	}
}

BaseType ParseBaseType(const std::string& name)
{
	if (name == LanguageTokens::TypeInt) return BaseType::Int;
	if (name == LanguageTokens::TypeUint) return BaseType::Uint;
	if (name == LanguageTokens::TypeReal) return BaseType::Real;
	if (name == LanguageTokens::TypeBool) return BaseType::Bool;
	if (name == LanguageTokens::TypeString) return BaseType::String;
	if (name == LanguageTokens::TypeVoid) return BaseType::Void;
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