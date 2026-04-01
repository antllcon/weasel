#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
	Constant = 0,
	Return,
	AddSingle,
	AddDouble,
	AddSNumber,
	SubtractSingle,
	SubtractDouble,
	MultiplySingle,
	MultiplyDouble,
	DivideSingle,
	DivideDouble,
	BitAndULittle,
	Jump,
	JumpIfFalse
};