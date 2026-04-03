#pragma once
#include <string>

class TypeInfo
{
public:
	virtual ~TypeInfo() = default;
	[[nodiscard]] virtual std::string GetName() const = 0;
	[[nodiscard]] virtual bool IsScalar() const = 0;
	// TODO: ещё что-то будет...

};