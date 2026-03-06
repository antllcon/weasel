#pragma once
#include <variant>

using Value = std::variant<double>;

Value AddValues(const Value& lhs, const Value& rhs);
Value SubtractValues(const Value& lhs, const Value& rhs);
Value MultiplyValues(const Value& lhs, const Value& rhs);
Value DivideValues(const Value& lhs, const Value& rhs);
Value NegateValue(const Value& value);