#include "ArrayTypeInfo.h"

#include "src/compiler/core/LanguageTokens.h"

ArrayTypeInfo::ArrayTypeInfo(std::shared_ptr<TypeInfo> elementType)
	: m_elementType(std::move(elementType))
{
}

std::string ArrayTypeInfo::GetName() const
{
	return std::string(LanguageTokens::KwArray) + "[" + m_elementType->GetName() + "]";
}

bool ArrayTypeInfo::IsScalar() const
{
	return false;
}

const std::shared_ptr<TypeInfo>& ArrayTypeInfo::GetElementType() const
{
	return m_elementType;
}