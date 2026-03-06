#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
	Constant = 0u,
	Add,
	Subtract,
	Multiply,
	Divide,
	Negate,
	Return = 255u
};