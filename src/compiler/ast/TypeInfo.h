#pragma once
#include <memory>
#include <string>

enum class BaseType
{
	Bitten, Little, Number, Longer,
	Single, Double,
	Boolen, String, Voided,
	Planar, Vector, Quadra, Linear, Matrix,
	Custom
};

enum class Signedness
{
	Signed, Unsigned, None
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
	ScalarTypeInfo(BaseType base, Signedness sign);

	[[nodiscard]] std::string GetName() const override;
	[[nodiscard]] bool IsScalar() const override;
	[[nodiscard]] BaseType GetBaseType() const;
	[[nodiscard]] Signedness GetSignedness() const;
	[[nodiscard]] bool IsInteger() const;
	[[nodiscard]] bool IsFloat() const;
	[[nodiscard]] bool IsVoided() const;

	[[nodiscard]] static std::shared_ptr<ScalarTypeInfo> Make(BaseType base, Signedness sign = Signedness::None);
	[[nodiscard]] static std::shared_ptr<ScalarTypeInfo> FromStrings(const std::string& sign, const std::string& typeName);

private:
	BaseType m_base;
	Signedness m_sign;
};