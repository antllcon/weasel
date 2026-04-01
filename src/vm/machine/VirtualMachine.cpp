// src/vm/machine/VirtualMachine.cpp
#include "VirtualMachine.h"
#include <stdexcept>

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

void ExecuteConstantInstruction(ExecutionContext& context)
{
	const uint8_t index = ReadByte(context);
	const Value constant = context.m_chunk.GetConstants()[index];
	Push(context, constant);
}

void ExecuteAddDoubleInstruction(const ExecutionContext& context)
{
	const double rhs = Pop(context).AsDouble();
	const double lhs = Pop(context).AsDouble();
	Push(context, Value(lhs + rhs));
}

void ExecuteSubtractDoubleInstruction(const ExecutionContext& context)
{
	const double rhs = Pop(context).AsDouble();
	const double lhs = Pop(context).AsDouble();
	Push(context, Value(lhs - rhs));
}

void ExecuteMultiplyDoubleInstruction(const ExecutionContext& context)
{
	const double rhs = Pop(context).AsDouble();
	const double lhs = Pop(context).AsDouble();
	Push(context, Value(lhs * rhs));
}

void ExecuteDivideDoubleInstruction(const ExecutionContext& context)
{
	const double rhs = Pop(context).AsDouble();
	const double lhs = Pop(context).AsDouble();
	Push(context, Value(lhs / rhs));
}

void ExecuteAddSingleInstruction(const ExecutionContext& context)
{
	const float rhs = Pop(context).AsSingle();
	const float lhs = Pop(context).AsSingle();
	Push(context, Value(lhs + rhs));
}

void ExecuteSubtractSingleInstruction(const ExecutionContext& context)
{
	const float rhs = Pop(context).AsSingle();
	const float lhs = Pop(context).AsSingle();
	Push(context, Value(lhs - rhs));
}

void ExecuteMultiplySingleInstruction(const ExecutionContext& context)
{
	const float rhs = Pop(context).AsSingle();
	const float lhs = Pop(context).AsSingle();
	Push(context, Value(lhs * rhs));
}

void ExecuteDivideSingleInstruction(const ExecutionContext& context)
{
	const float rhs = Pop(context).AsSingle();
	const float lhs = Pop(context).AsSingle();
	Push(context, Value(lhs / rhs));
}

void ExecuteAddSNumberInstruction(const ExecutionContext& context)
{
	const int32_t rhs = Pop(context).AsSNumber();
	const int32_t lhs = Pop(context).AsSNumber();
	Push(context, Value(static_cast<int32_t>(static_cast<uint32_t>(lhs) + static_cast<uint32_t>(rhs))));
}

void Run(ExecutionContext& context)
{
	for (;;)
	{
		const auto instruction = static_cast<OpCode>(ReadByte(context));

		switch (instruction)
		{
		case OpCode::Constant: {
			ExecuteConstantInstruction(context);
			break;
		}
		case OpCode::AddDouble: {
			ExecuteAddDoubleInstruction(context);
			break;
		}
		case OpCode::SubtractDouble: {
			ExecuteSubtractDoubleInstruction(context);
			break;
		}
		case OpCode::MultiplyDouble: {
			ExecuteMultiplyDoubleInstruction(context);
			break;
		}
		case OpCode::DivideDouble: {
			ExecuteDivideDoubleInstruction(context);
			break;
		}
		case OpCode::AddSingle: {
			ExecuteAddSingleInstruction(context);
			break;
		}
		case OpCode::SubtractSingle: {
			ExecuteSubtractSingleInstruction(context);
			break;
		}
		case OpCode::MultiplySingle: {
			ExecuteMultiplySingleInstruction(context);
			break;
		}
		case OpCode::DivideSingle: {
			ExecuteDivideSingleInstruction(context);
			break;
		}
		case OpCode::AddSNumber: {
			ExecuteAddSNumberInstruction(context);
			break;
		}
		case OpCode::BitAndULittle:
		case OpCode::Jump:
		case OpCode::JumpIfFalse: {
			AssertIsImplemented(false);
			break;
		}
		case OpCode::Return: {
			return;
		}
		default: {
			AssertIsUnknownOpCode(true);
		}
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

	ExecutionContext context{
		chunk,
		m_stack,
		0};

	Run(context);
}

Value VirtualMachine::Peek(size_t distance) const
{
	AssertIsStackDistanceValid(distance < m_stack.size());
	return m_stack[m_stack.size() - 1 - distance];
}