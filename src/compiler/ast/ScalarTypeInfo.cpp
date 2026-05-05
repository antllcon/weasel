#include "TypeInfo.h"
#include <stdexcept>
#include <unordered_map>

namespace
{
std::string FormatName(BaseType base, Signedness sign)
{
	static const std::unordered_map<int, std::string> names = {
		{static_cast<int>(BaseType::Bitten), "bitten"},
		{static_cast<int>(BaseType::Little), "little"},
		{static_cast<int>(BaseType::Number), "number"},
		{static_cast<int>(BaseType::Longer), "longer"},
		{static_cast<int>(BaseType::Single), "single"},
		{static_cast<int>(BaseType::Double), "double"},
		{static_cast<int>(BaseType::Boolen), "boolen"},
		{static_cast<int>(BaseType::String), "string"},
		{static_cast<int>(BaseType::Voided), "voided"},
		{static_cast<int>(BaseType::Planar), "planar"},
		{static_cast<int>(BaseType::Vector), "vector"},
		{static_cast<int>(BaseType::Quadra), "quadra"},
		{static_cast<int>(BaseType::Linear), "linear"},
		{static_cast<int>(BaseType::Matrix), "matrix"},
	};
	std::string prefix;
	if (sign == Signedness::Signed) prefix = "s ";
	if (sign == Signedness::Unsigned) prefix = "u ";
	const auto it = names.find(static_cast<int>(base));
	return prefix + (it != names.end() ? it->second : "custom");
}

BaseType ParseBaseType(const std::string& name)
{
	static const std::unordered_map<std::string, BaseType> map = {
		{"bitten", BaseType::Bitten}, {"little", BaseType::Little},
		{"number", BaseType::Number}, {"longer", BaseType::Longer},
		{"single", BaseType::Single}, {"double", BaseType::Double},
		{"boolen", BaseType::Boolen}, {"string", BaseType::String},
		{"voided", BaseType::Voided}, {"planar", BaseType::Planar},
		{"vector", BaseType::Vector}, {"quadra", BaseType::Quadra},
		{"acolor", BaseType::Quadra}, {"linear", BaseType::Linear},
		{"matrix", BaseType::Matrix},
	};
	const auto it = map.find(name);
	if (it == map.end())
	{
		throw std::runtime_error("Неизвестное имя типа: " + name);
	}
	return it->second;
}
} // namespace

ScalarTypeInfo::ScalarTypeInfo(BaseType base, Signedness sign)
	: m_base(base)
	, m_sign(sign)
{
}

std::string ScalarTypeInfo::GetName() const
{
	return FormatName(m_base, m_sign);
}

bool ScalarTypeInfo::IsScalar() const
{
	return true;
}

BaseType ScalarTypeInfo::GetBaseType() const
{
	return m_base;
}

Signedness ScalarTypeInfo::GetSignedness() const
{
	return m_sign;
}

bool ScalarTypeInfo::IsInteger() const
{
	return m_base == BaseType::Bitten || m_base == BaseType::Little
		|| m_base == BaseType::Number || m_base == BaseType::Longer;
}

bool ScalarTypeInfo::IsFloat() const
{
	return m_base == BaseType::Single || m_base == BaseType::Double;
}

bool ScalarTypeInfo::IsVoided() const
{
	return m_base == BaseType::Voided;
}

std::shared_ptr<ScalarTypeInfo> ScalarTypeInfo::Make(BaseType base, Signedness sign)
{
	return std::make_shared<ScalarTypeInfo>(base, sign);
}

std::shared_ptr<ScalarTypeInfo> ScalarTypeInfo::FromStrings(const std::string& sign, const std::string& typeName)
{
	Signedness s = Signedness::None;
	if (sign == "s") s = Signedness::Signed;
	if (sign == "u") s = Signedness::Unsigned;
	return std::make_shared<ScalarTypeInfo>(ParseBaseType(typeName), s);
}
