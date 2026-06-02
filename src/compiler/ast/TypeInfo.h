#pragma once

#include <string>

enum class BaseType
{
	Int,
	Uint,
	Real,
	Bool,
	String,
	Void,
	Custom
};

class TypeInfo
{
public:
	virtual ~TypeInfo() = default;
	virtual std::string GetName() const = 0;
	virtual bool IsScalar() const = 0;
};