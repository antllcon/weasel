#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>

class Value
{
public:
	Value();

	template <typename T>
	explicit Value(T value)
		: m_data(0)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(sizeof(T) <= sizeof(uint64_t));
		std::memcpy(&m_data, &value, sizeof(T));
	}

	template <typename T>
	T As() const
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(sizeof(T) <= sizeof(uint64_t));
		T result;
		std::memcpy(&result, &m_data, sizeof(T));
		return result;
	}

	uint64_t AsRaw() const;

private:
	uint64_t m_data;
};