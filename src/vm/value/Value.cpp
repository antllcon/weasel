#include "Value.h"
#include <cstring>

Value::Value()
	: m_data(0)
{
}

Value::Value(uint64_t value)
	: m_data(value)
{
}

Value::Value(double value)
	: m_data(0)
{
	std::memcpy(&m_data, &value, sizeof(double));
}

Value::Value(float value)
	: m_data(0)
{
	std::memcpy(&m_data, &value, sizeof(float));
}

Value::Value(int32_t value)
	: m_data(0)
{
	std::memcpy(&m_data, &value, sizeof(int32_t));
}

uint64_t Value::AsRaw() const
{
	return m_data;
}

double Value::AsDouble() const
{
	double result;
	std::memcpy(&result, &m_data, sizeof(double));
	return result;
}

float Value::AsSingle() const
{
	float result;
	std::memcpy(&result, &m_data, sizeof(float));
	return result;
}

int32_t Value::AsSNumber() const
{
	int32_t result;
	std::memcpy(&result, &m_data, sizeof(int32_t));
	return result;
}