#include "VirtualMachine.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace
{
inline constexpr uint16_t STACK_RESERVE = 256u;

struct ExecutionContext
{
	const Chunk& m_chunk;
	std::vector<Value>& m_stack;
	uint32_t m_ip;
};

void AssertIsStackNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Стек пуст, невозможно извлечь значение");
	}
}

void AssertIsIpValid(bool isValid)
{
	if (!isValid)
	{
		throw std::runtime_error("Указатель инструкций вышел за пределы памяти");
	}
}

void AssertIsUnknownOpCode(bool isUnknown)
{
	if (isUnknown)
	{
		throw std::runtime_error("Обнаружен неизвестный код операции");
	}
}

void AssertIsStackDistanceValid(bool isValid)
{
	if (!isValid)
	{
		throw std::runtime_error("Попытка доступа к элементу за пределами стека");
	}
}

void AssertIsImplemented(bool isImplemented)
{
	if (!isImplemented)
	{
		throw std::runtime_error("Инструкция еще не реализована в виртуальной машине");
	}
}

void AssertIsNotDivisionByZero(bool isZero)
{
	if (isZero)
	{
		throw std::runtime_error("Деление на ноль");
	}
}

uint8_t ReadByte(ExecutionContext& context)
{
	AssertIsIpValid(context.m_ip < context.m_chunk.GetCode().size());
	return context.m_chunk.GetCode()[context.m_ip++];
}

void Push(const ExecutionContext& context, const Value& value)
{
	context.m_stack.push_back(value);
}

Value Pop(const ExecutionContext& context)
{
	AssertIsStackNotEmpty(context.m_stack.empty());
	const Value value = context.m_stack.back();
	context.m_stack.pop_back();
	return value;
}

template <typename T>
void ExecuteAdd(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs + rhs)));
}

template <typename T>
void ExecuteSub(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs - rhs)));
}

template <typename T>
void ExecuteMul(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs * rhs)));
}

template <typename T>
void ExecuteDiv(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	AssertIsNotDivisionByZero(rhs == 0);
	Push(context, Value(static_cast<T>(lhs / rhs)));
}

template <typename T>
void ExecuteRem(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	AssertIsNotDivisionByZero(rhs == 0);

	if constexpr (std::is_floating_point_v<T>)
	{
		Push(context, Value(static_cast<T>(std::fmod(lhs, rhs))));
	}
	else
	{
		Push(context, Value(static_cast<T>(lhs % rhs)));
	}
}

template <typename T>
void ExecuteEq(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(lhs == rhs));
}

template <typename T>
void ExecuteLt(const ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(lhs < rhs));
}

template <typename T>
void ExecuteBitAnd(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs & rhs)));
}

template <typename T>
void ExecuteBitOr(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs | rhs)));
}

template <typename T>
void ExecuteBitXor(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs ^ rhs)));
}

template <typename T>
void ExecuteBitNot(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T operand = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(~operand)));
}

template <typename T>
void ExecuteShl(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs << rhs)));
}

template <typename T>
void ExecuteShr(const ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs >> rhs)));
}

void ExecuteConstantInstruction(ExecutionContext& context)
{
	const uint8_t index = ReadByte(context);
	const Value constant = context.m_chunk.GetConstants()[index];
	Push(context, constant);
}

void ExecuteDupInstruction(ExecutionContext& context)
{
	AssertIsStackNotEmpty(context.m_stack.empty());
	Push(context, context.m_stack.back());
}

void Run(ExecutionContext& context)
{
	for (;;)
	{
		const auto instruction = static_cast<OpCode>(ReadByte(context));

		switch (instruction)
		{
		case OpCode::Constant:
			ExecuteConstantInstruction(context);
			break;

		case OpCode::AddI8:
			ExecuteAdd<int8_t>(context);
			break;
		case OpCode::AddU8:
			ExecuteAdd<uint8_t>(context);
			break;
		case OpCode::AddI16:
			ExecuteAdd<int16_t>(context);
			break;
		case OpCode::AddU16:
			ExecuteAdd<uint16_t>(context);
			break;
		case OpCode::AddI32:
			ExecuteAdd<int32_t>(context);
			break;
		case OpCode::AddU32:
			ExecuteAdd<uint32_t>(context);
			break;
		case OpCode::AddI64:
			ExecuteAdd<int64_t>(context);
			break;
		case OpCode::AddU64:
			ExecuteAdd<uint64_t>(context);
			break;
		case OpCode::AddF32:
			ExecuteAdd<float>(context);
			break;
		case OpCode::AddF64:
			ExecuteAdd<double>(context);
			break;

		case OpCode::SubI8:
			ExecuteSub<int8_t>(context);
			break;
		case OpCode::SubU8:
			ExecuteSub<uint8_t>(context);
			break;
		case OpCode::SubI16:
			ExecuteSub<int16_t>(context);
			break;
		case OpCode::SubU16:
			ExecuteSub<uint16_t>(context);
			break;
		case OpCode::SubI32:
			ExecuteSub<int32_t>(context);
			break;
		case OpCode::SubU32:
			ExecuteSub<uint32_t>(context);
			break;
		case OpCode::SubI64:
			ExecuteSub<int64_t>(context);
			break;
		case OpCode::SubU64:
			ExecuteSub<uint64_t>(context);
			break;
		case OpCode::SubF32:
			ExecuteSub<float>(context);
			break;
		case OpCode::SubF64:
			ExecuteSub<double>(context);
			break;

		case OpCode::MulI8:
			ExecuteMul<int8_t>(context);
			break;
		case OpCode::MulU8:
			ExecuteMul<uint8_t>(context);
			break;
		case OpCode::MulI16:
			ExecuteMul<int16_t>(context);
			break;
		case OpCode::MulU16:
			ExecuteMul<uint16_t>(context);
			break;
		case OpCode::MulI32:
			ExecuteMul<int32_t>(context);
			break;
		case OpCode::MulU32:
			ExecuteMul<uint32_t>(context);
			break;
		case OpCode::MulI64:
			ExecuteMul<int64_t>(context);
			break;
		case OpCode::MulU64:
			ExecuteMul<uint64_t>(context);
			break;
		case OpCode::MulF32:
			ExecuteMul<float>(context);
			break;
		case OpCode::MulF64:
			ExecuteMul<double>(context);
			break;

		case OpCode::DivI8:
			ExecuteDiv<int8_t>(context);
			break;
		case OpCode::DivU8:
			ExecuteDiv<uint8_t>(context);
			break;
		case OpCode::DivI16:
			ExecuteDiv<int16_t>(context);
			break;
		case OpCode::DivU16:
			ExecuteDiv<uint16_t>(context);
			break;
		case OpCode::DivI32:
			ExecuteDiv<int32_t>(context);
			break;
		case OpCode::DivU32:
			ExecuteDiv<uint32_t>(context);
			break;
		case OpCode::DivI64:
			ExecuteDiv<int64_t>(context);
			break;
		case OpCode::DivU64:
			ExecuteDiv<uint64_t>(context);
			break;
		case OpCode::DivF32:
			ExecuteDiv<float>(context);
			break;
		case OpCode::DivF64:
			ExecuteDiv<double>(context);
			break;

		case OpCode::EqI8:
			ExecuteEq<int8_t>(context);
			break;
		case OpCode::EqU8:
			ExecuteEq<uint8_t>(context);
			break;
		case OpCode::EqI16:
			ExecuteEq<int16_t>(context);
			break;
		case OpCode::EqU16:
			ExecuteEq<uint16_t>(context);
			break;
		case OpCode::EqI32:
			ExecuteEq<int32_t>(context);
			break;
		case OpCode::EqU32:
			ExecuteEq<uint32_t>(context);
			break;
		case OpCode::EqI64:
			ExecuteEq<int64_t>(context);
			break;
		case OpCode::EqU64:
			ExecuteEq<uint64_t>(context);
			break;
		case OpCode::EqF32:
			ExecuteEq<float>(context);
			break;
		case OpCode::EqF64:
			ExecuteEq<double>(context);
			break;

		case OpCode::LtI8:
			ExecuteLt<int8_t>(context);
			break;
		case OpCode::LtU8:
			ExecuteLt<uint8_t>(context);
			break;
		case OpCode::LtI16:
			ExecuteLt<int16_t>(context);
			break;
		case OpCode::LtU16:
			ExecuteLt<uint16_t>(context);
			break;
		case OpCode::LtI32:
			ExecuteLt<int32_t>(context);
			break;
		case OpCode::LtU32:
			ExecuteLt<uint32_t>(context);
			break;
		case OpCode::LtI64:
			ExecuteLt<int64_t>(context);
			break;
		case OpCode::LtU64:
			ExecuteLt<uint64_t>(context);
			break;
		case OpCode::LtF32:
			ExecuteLt<float>(context);
			break;
		case OpCode::LtF64:
			ExecuteLt<double>(context);
			break;

		case OpCode::BitAnd:
			ExecuteBitAnd<uint64_t>(context);
			break;
		case OpCode::BitOr:
			ExecuteBitOr<uint64_t>(context);
			break;
		case OpCode::BitXor:
			ExecuteBitXor<uint64_t>(context);
			break;
		case OpCode::BitNot:
			ExecuteBitNot<uint64_t>(context);
			break;
		case OpCode::Shl:
			ExecuteShl<uint64_t>(context);
			break;
		case OpCode::Shr:
			ExecuteShr<uint64_t>(context);
			break;

		case OpCode::Pop:
			Pop(context);
			break;
		case OpCode::Dup:
			ExecuteDupInstruction(context);
			break;

		case OpCode::LoadLocal:
		case OpCode::StoreLocal:
		case OpCode::Jump:
		case OpCode::JumpIfFalse:
		case OpCode::JumpIfTrue:
		case OpCode::Call:
			AssertIsImplemented(false);
			break;

		case OpCode::Return:
			return;

		default:
			AssertIsUnknownOpCode(true);
		}
	}
}
} // namespace

VirtualMachine::VirtualMachine()
{
	m_stack.reserve(STACK_RESERVE);
}

void VirtualMachine::Interpret(const Chunk& chunk)
{
	m_stack.clear();

	ExecutionContext context{chunk, m_stack, 0};

	Run(context);
}

Value VirtualMachine::Peek(size_t distance) const
{
	AssertIsStackDistanceValid(distance < m_stack.size());
	return m_stack[m_stack.size() - 1 - distance];
}