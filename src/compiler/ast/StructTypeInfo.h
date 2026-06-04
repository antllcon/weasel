#pragma once
#include "FieldDecl.h"
#include "TypeInfo.h"
#include <string>
#include <vector>

class StructTypeInfo final : public TypeInfo
{
public:
	StructTypeInfo(std::string name, std::vector<FieldDecl> fields);

	[[nodiscard]] std::string GetName() const override;
	[[nodiscard]] bool IsScalar() const override;

	[[nodiscard]] const std::vector<FieldDecl>& GetFields() const;
	[[nodiscard]] uint32_t GetFieldCount() const;

private:
	std::string m_name;
	std::vector<FieldDecl> m_fields;
};
