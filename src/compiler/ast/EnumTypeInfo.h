#pragma once

#include "TypeInfo.h"
#include <string>
#include <vector>

class EnumTypeInfo final : public TypeInfo
{
public:
	EnumTypeInfo(std::string name, std::vector<std::string> fields);

	std::string GetName() const override;
	bool IsScalar() const override;

	const std::vector<std::string>& GetFields() const;

private:
	std::string m_name;
	std::vector<std::string> m_fields;
};