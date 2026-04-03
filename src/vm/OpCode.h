#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
	Constant = 0,

	AddI8, AddU8, AddI16, AddU16, AddI32, AddU32, AddI64, AddU64, AddF32, AddF64,
	SubI8, SubU8, SubI16, SubU16, SubI32, SubU32, SubI64, SubU64, SubF32, SubF64,
	MulI8, MulU8, MulI16, MulU16, MulI32, MulU32, MulI64, MulU64, MulF32, MulF64,
	DivI8, DivU8, DivI16, DivU16, DivI32, DivU32, DivI64, DivU64, DivF32, DivF64,
	RemI8, RemU8, RemI16, RemU16, RemI32, RemU32, RemI64, RemU64,

	EqI8, EqU8, EqI16, EqU16, EqI32, EqU32, EqI64, EqU64, EqF32, EqF64,
	LtI8, LtU8, LtI16, LtU16, LtI32, LtU32, LtI64, LtU64, LtF32, LtF64,

	BitAnd, BitOr, BitXor, BitNot, Shl, Shr,

	LoadLocal,
	StoreLocal,
	Pop,
	Dup,

	Jump,
	JumpIfFalse,
	JumpIfTrue,

	AllocateStruct,
	GetField,
	StoreField,

	AllocateArray,
	LoadElement,
	StoreElement,

	Retain,
	Release,

	Call,
	Return,

	AllocateClosure,
	CallClosure,

	CallNative,

	LoadString,
	Panic
};