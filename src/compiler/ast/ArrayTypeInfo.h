#pragma once

#include "TypeInfo.h"

#include <memory>

class ArrayTypeInfo final : public TypeInfo
{
public:
	explicit ArrayTypeInfo(std::shared_ptr<TypeInfo> elementType);

	std::string GetName() const override;
	bool IsScalar() const override;
	const std::shared_ptr<TypeInfo>& GetElementType() const;

private:
	std::shared_ptr<TypeInfo> m_elementType;
};