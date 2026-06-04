#pragma once
#include "src/compiler/ast/EnumTypeInfo.h"
#include "src/compiler/ast/StructTypeInfo.h"
#include "src/compiler/ast/TypeInfo.h"
#include <memory>
#include <string>
#include <unordered_map>

class TypeResolver
{
public:
	void RegisterEnum(const std::string& name, std::shared_ptr<EnumTypeInfo> enumType);
	void RegisterStruct(const std::string& name, std::shared_ptr<StructTypeInfo> structType);

	std::shared_ptr<TypeInfo> Resolve(const std::string& typeName) const;
	std::shared_ptr<EnumTypeInfo> GetEnum(const std::string& name) const;
	std::shared_ptr<StructTypeInfo> GetStruct(const std::string& name) const;
	bool IsEnum(const std::string& name) const;
	bool IsStruct(const std::string& name) const;

private:
	std::unordered_map<std::string, std::shared_ptr<EnumTypeInfo>> m_enums;
	std::unordered_map<std::string, std::shared_ptr<StructTypeInfo>> m_structs;
};