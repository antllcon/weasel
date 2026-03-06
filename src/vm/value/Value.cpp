#include "Value.h"
#include <stdexcept>

namespace
{
	void AssertIsSameArithmeticType(bool condition)
	{
		if (!condition)
		{
			throw std::runtime_error("Операнды должны быть числового типа");
		}
	}

	void AssertIsNotZero(double value)
	{
		if (value == 0.0)
		{
			throw std::runtime_error("Деление на ноль невозможно");
		}
	}
}

Value AddValues(const Value& lhs, const Value& rhs)
{
	return std::visit([](auto&& l, auto&& r) -> Value
	{
		using L = std::decay_t<decltype(l)>;
		using R = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return l + r;
		}

		AssertIsSameArithmeticType(false);
		return {};
	}, lhs, rhs);
}

Value SubtractValues(const Value& lhs, const Value& rhs)
{
	return std::visit([](auto&& l, auto&& r) -> Value
	{
		using L = std::decay_t<decltype(l)>;
		using R = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return l - r;
		}

		AssertIsSameArithmeticType(false);
		return {};
	}, lhs, rhs);
}

Value MultiplyValues(const Value& lhs, const Value& rhs)
{
	return std::visit([](auto&& l, auto&& r) -> Value
	{
		using L = std::decay_t<decltype(l)>;
		using R = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			return l * r;
		}

		AssertIsSameArithmeticType(false);
		return {};
	}, lhs, rhs);
}

Value DivideValues(const Value& lhs, const Value& rhs)
{
	return std::visit([](auto&& l, auto&& r) -> Value
	{
		using L = std::decay_t<decltype(l)>;
		using R = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<L, double> && std::is_same_v<R, double>)
		{
			AssertIsNotZero(r);
			return l / r;
		}

		AssertIsSameArithmeticType(false);
		return {};
	}, lhs, rhs);
}

Value NegateValue(const Value& value)
{
	return std::visit([](auto&& v) -> Value
	{
		using V = std::decay_t<decltype(v)>;

		if constexpr (std::is_same_v<V, double>)
		{
			return -v;
		}

		AssertIsSameArithmeticType(false);
		return {};
	}, value);
}