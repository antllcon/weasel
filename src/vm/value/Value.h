#pragma once

#include <string>
#include <variant>

class Value
{
public:
	Value();
	Value(double value);
	Value(const std::string& value);

	Value operator+(const Value& rhs) const;
	Value operator-(const Value& rhs) const;
	Value operator*(const Value& rhs) const;
	Value operator/(const Value& rhs) const;
	Value operator-() const;
	bool operator==(const Value& rhs) const;

	template <typename Visitor>
	auto visit(Visitor&& visitor) const
	{
		return std::visit(std::forward<Visitor>(visitor), m_data);
	}

private:
	std::variant<double, std::string> m_data;
};