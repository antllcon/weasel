#include "Value.h"
#include <stdexcept>

namespace
{
void AssertIsOperationSupported(bool condition)
{
	if (!condition)
	{
		throw std::runtime_error("Операция не поддерживается для данных типов");
	}
}

void AssertIsNotZero(double value)
{
	if (value == 0.0)
	{
		throw std::runtime_error("Деление на ноль невозможно");
	}
}
} // namespace

Value::Value()
	: m_data(0.0)
{
}

Value::Value(double value)
	: m_data(value)
{
}

Value::Value(const std::string& value)
	: m_data(value)
{
}

Value Value::operator+(const Value& rhs) const
{
	return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> Value {
		using L = std::decay_t<T0>;
		using R = std::decay_t<T1>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return Value(l + r);
		}
		else if constexpr (std::is_same_v<L, std::string> && std::is_same_v<R, std::string>)
		{
			return Value(l + r);
		}

		AssertIsOperationSupported(false);
		return Value();
	},
		m_data,
		rhs.m_data);
}

Value Value::operator-(const Value& rhs) const
{
	return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> Value {
		using L = std::decay_t<T0>;
		using R = std::decay_t<T1>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return Value(l - r);
		}

		AssertIsOperationSupported(false);
		return Value();
	},
		m_data,
		rhs.m_data);
}

Value Value::operator*(const Value& rhs) const
{
	return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> Value {
		using L = std::decay_t<T0>;
		using R = std::decay_t<T1>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return Value(l * r);
		}

		AssertIsOperationSupported(false);
		return Value();
	},
		m_data,
		rhs.m_data);
}

Value Value::operator/(const Value& rhs) const
{
	return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> Value {
		using L = std::decay_t<T0>;
		using R = std::decay_t<T1>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			AssertIsNotZero(r);
			return Value(l / r);
		}

		AssertIsOperationSupported(false);
		return Value();
	},
		m_data,
		rhs.m_data);
}

Value Value::operator-() const
{
	return std::visit([]<typename T0>(T0&& v) -> Value {
		using V = std::decay_t<T0>;

		if constexpr (std::is_same_v<V, double>)
		{
			return Value(-v);
		}

		AssertIsOperationSupported(false);
		return Value();
	},
		m_data);
}

bool Value::operator==(const Value& rhs) const
{
	return m_data == rhs.m_data;
}