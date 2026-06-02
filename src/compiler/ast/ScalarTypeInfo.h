#pragma once

#include "TypeInfo.h"
#include <memory>

class ScalarTypeInfo final : public TypeInfo
{
public:
	explicit ScalarTypeInfo(BaseType base);

	std::string GetName() const override;
	bool IsScalar() const override;
	BaseType GetBaseType() const;

	bool IsInteger() const;
	bool IsFloat() const;
	bool IsVoid() const;
	static std::shared_ptr<ScalarTypeInfo> Make(BaseType base);
	static std::shared_ptr<ScalarTypeInfo> FromString(const std::string& typeName);

private:
	BaseType m_base;
};