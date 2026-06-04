#pragma once
#include "src/compiler/ast/EnumTypeInfo.h"
#include "src/compiler/ast/TypeInfo.h"
#include <memory>
#include <string>
#include <unordered_map>

class TypeResolver
{
public:
	void RegisterEnum(const std::string& name, std::shared_ptr<EnumTypeInfo> enumType);

	std::shared_ptr<TypeInfo> Resolve(const std::string& typeName) const;
	std::shared_ptr<EnumTypeInfo> GetEnum(const std::string& name) const;
	bool IsEnum(const std::string& name) const;

private:
	std::unordered_map<std::string, std::shared_ptr<EnumTypeInfo>> m_enums;
};