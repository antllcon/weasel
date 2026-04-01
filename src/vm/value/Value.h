#pragma once
#include <cstdint>

class Value
{
public:
	Value();
	explicit Value(uint64_t value);
	explicit Value(double value);
	explicit Value(float value);
	explicit Value(int32_t value);

	uint64_t AsRaw() const;
	double AsDouble() const;
	float AsSingle() const;
	int32_t AsSNumber() const;

private:
	uint64_t m_data;
};