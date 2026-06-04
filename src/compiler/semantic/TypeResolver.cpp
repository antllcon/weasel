#include "TypeResolver.h"
#include "src/compiler/ast/ArrayTypeInfo.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/core/LanguageTokens.h"

void TypeResolver::RegisterEnum(const std::string& name, std::shared_ptr<EnumTypeInfo> enumType)
{
	m_enums[name] = std::move(enumType);
}

void TypeResolver::RegisterStruct(const std::string& name, std::shared_ptr<StructTypeInfo> structType)
{
	m_structs[name] = std::move(structType);
}

std::shared_ptr<StructTypeInfo> TypeResolver::GetStruct(const std::string& name) const
{
	const auto it = m_structs.find(name);
	return it != m_structs.end() ? it->second : nullptr;
}

bool TypeResolver::IsStruct(const std::string& name) const
{
	return m_structs.contains(name);
}

std::shared_ptr<TypeInfo> TypeResolver::Resolve(const std::string& typeName) const
{
	if (m_enums.contains(typeName))
	{
		return m_enums.at(typeName);
	}

	if (m_structs.contains(typeName))
	{
		return m_structs.at(typeName);
	}

	if (typeName.starts_with(LanguageTokens::KwArray) && typeName.find('[') != std::string::npos)
	{
		const size_t start = typeName.find('[') + 1;
		const size_t end = typeName.find_last_of(']');
		const std::string inner = typeName.substr(start, end - start);
		return std::make_shared<ArrayTypeInfo>(Resolve(inner));
	}

	return ScalarTypeInfo::FromString(typeName);
}

std::shared_ptr<EnumTypeInfo> TypeResolver::GetEnum(const std::string& name) const
{
	auto it = m_enums.find(name);
	return it != m_enums.end() ? it->second : nullptr;
}

bool TypeResolver::IsEnum(const std::string& name) const
{
	return m_enums.contains(name);
}