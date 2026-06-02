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

		if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
		{
			const int64_t promoted = value;
			std::memcpy(&m_data, &promoted, sizeof(int64_t));
		}
		else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
		{
			const uint64_t promoted = value;
			std::memcpy(&m_data, &promoted, sizeof(uint64_t));
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			const double promoted = value;
			std::memcpy(&m_data, &promoted, sizeof(double));
		}
		else
		{
			std::memcpy(&m_data, &value, sizeof(T));
		}
	}

	template <typename T>
	T As() const
	{
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(sizeof(T) <= sizeof(uint64_t));

		if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
		{
			int64_t result;
			std::memcpy(&result, &m_data, sizeof(int64_t));
			return static_cast<T>(result);
		}
		else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
		{
			uint64_t result;
			std::memcpy(&result, &m_data, sizeof(uint64_t));
			return static_cast<T>(result);
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			double result;
			std::memcpy(&result, &m_data, sizeof(double));
			return static_cast<T>(result);
		}
		else
		{
			T result;
			std::memcpy(&result, &m_data, sizeof(T));
			return result;
		}
	}

	[[nodiscard]] uint64_t AsRaw() const;

private:
	uint64_t m_data;
};