#include "VirtualMachine.h"
#include <stdexcept>

namespace
{
inline constexpr size_t STACK_RESERVE = 256;

struct ExecutionContext
{
	const Chunk& m_chunk;
	std::vector<Value>& m_stack;
	Value& m_lastResult;
	size_t m_ip;
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

uint8_t ReadByte(ExecutionContext& context)
{
	AssertIsIpValid(context.m_ip < context.m_chunk.GetCode().size());
	return context.m_chunk.GetCode()[context.m_ip++];
}

void Push(ExecutionContext& context, const Value& value)
{
	context.m_stack.push_back(value);
}

Value Pop(ExecutionContext& context)
{
	AssertIsStackNotEmpty(context.m_stack.empty());
	Value value = context.m_stack.back();
	context.m_stack.pop_back();
	return value;
}

void ExecuteConstantInstruction(ExecutionContext& context)
{
	const uint8_t index = ReadByte(context);
	const Value constant = context.m_chunk.GetConstants()[index];
	Push(context, constant);
}

void ExecuteReturnInstruction(ExecutionContext& context)
{
	if (!context.m_stack.empty())
	{
		context.m_lastResult = Pop(context);
	}
}

void ExecuteAddInstruction(ExecutionContext& context)
{
	const Value rhs = Pop(context);
	const Value lhs = Pop(context);
	Push(context, AddValues(lhs, rhs));
}

void ExecuteSubtractInstruction(ExecutionContext& context)
{
	const Value rhs = Pop(context);
	const Value lhs = Pop(context);
	Push(context, SubtractValues(lhs, rhs));
}

void ExecuteMultiplyInstruction(ExecutionContext& context)
{
	const Value rhs = Pop(context);
	const Value lhs = Pop(context);
	Push(context, MultiplyValues(lhs, rhs));
}

void ExecuteDivideInstruction(ExecutionContext& context)
{
	const Value rhs = Pop(context);
	const Value lhs = Pop(context);
	Push(context, DivideValues(lhs, rhs));
}

void ExecuteNegateInstruction(ExecutionContext& context)
{
	const Value value = Pop(context);
	Push(context, NegateValue(value));
}

void Run(ExecutionContext& context)
{
	for (;;)
	{
		const OpCode instruction = static_cast<OpCode>(ReadByte(context));

		switch (instruction)
		{
		case OpCode::Constant: {
			ExecuteConstantInstruction(context);
			break;
		}
		case OpCode::Add: {
			ExecuteAddInstruction(context);
			break;
		}
		case OpCode::Subtract: {
			ExecuteSubtractInstruction(context);
			break;
		}
		case OpCode::Multiply: {
			ExecuteMultiplyInstruction(context);
			break;
		}
		case OpCode::Divide: {
			ExecuteDivideInstruction(context);
			break;
		}
		case OpCode::Negate: {
			ExecuteNegateInstruction(context);
			break;
		}
		case OpCode::Return: {
			ExecuteReturnInstruction(context);
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
	: m_lastResult(0.0)
{
	m_stack.reserve(STACK_RESERVE);
}

VirtualMachine::~VirtualMachine()
{
}

void VirtualMachine::Interpret(const Chunk& chunk)
{
	m_stack.clear();

	ExecutionContext context{
		chunk,
		m_stack,
		m_lastResult,
		0};

	Run(context);
}

Value VirtualMachine::GetLastResult() const
{
	return m_lastResult;
}