#include "Value.h"

Value::Value()
	: m_data(0)
{
}

uint64_t Value::AsRaw() const
{
	return m_data;
}