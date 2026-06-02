#pragma once
#include <memory>
#include <string>

namespace TypeKeyword
{
inline constexpr std::string_view Int = "int";
inline constexpr std::string_view Uint = "uint";
inline constexpr std::string_view Real = "real";
inline constexpr std::string_view Bool = "bool";
inline constexpr std::string_view String = "string";
inline constexpr std::string_view Void = "void";
} // namespace TypeKeyword

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
	[[nodiscard]] virtual std::string GetName() const = 0;
	[[nodiscard]] virtual bool IsScalar() const = 0;
};

class ScalarTypeInfo final : public TypeInfo
{
public:
	explicit ScalarTypeInfo(BaseType base);

	[[nodiscard]] std::string GetName() const override;
	[[nodiscard]] bool IsScalar() const override;
	[[nodiscard]] BaseType GetBaseType() const;

	[[nodiscard]] bool IsInteger() const;
	[[nodiscard]] bool IsFloat() const;
	[[nodiscard]] bool IsVoid() const;

	[[nodiscard]] static std::shared_ptr<ScalarTypeInfo> Make(BaseType base);
	[[nodiscard]] static std::shared_ptr<ScalarTypeInfo> FromString(const std::string& typeName);

private:
	BaseType m_base;
};