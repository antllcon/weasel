#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
	Constant = 0,

	AddInt, AddUint, AddReal,
	SubInt, SubUint, SubReal,
	MulInt, MulUint, MulReal,
	DivInt, DivUint, DivReal,
	RemInt, RemUint, RemReal,

	EqInt, EqUint, EqReal,
	LtInt, LtUint, LtReal,

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