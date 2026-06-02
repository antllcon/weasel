#include "VirtualMachine.h"
#include "src/compiler/vm/chunk/Chunk.h"
#include "src/compiler/vm/exception/VmException.h"
#include "src/compiler/vm/memory/HeapObject.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace
{
inline constexpr uint32_t MAX_STACK = 65536u;
inline constexpr uint16_t MAX_FRAMES = 256u;

struct ExecutionContext
{
	const Chunk& chunk;
	std::vector<Value>& stack;
	std::vector<VirtualMachine::CallFrame>& frames;
	const std::vector<VirtualMachine::NativeCallback>& natives;
	HeapTracker& tracker;
	uint32_t& stackTop;
	uint32_t ip;
	uint32_t stackOffset;
	std::vector<std::unique_ptr<std::string>>& dynamicStrings;
};

uint32_t ExtractCurrentLine(const ExecutionContext& context)
{
	const uint32_t errorIp = context.ip > 0 ? context.ip - 1 : 0;
	return context.chunk.GetLine(errorIp);
}

void AssertIsStringNotNull(const HeapObject* object, const ExecutionContext& context)
{
	if (!object)
	{
		throw VmException(
			"E_VM_NULL_STRING",
			"Попытка операции над неинициализированной строкой",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsNotDivisionOverflow(bool isOverflow, const ExecutionContext& context)
{
	if (isOverflow)
	{
		throw VmException(
			"E_VM_DIV_OVERFLOW",
			"Арифметическое переполнение при делении (INT_MIN / -1)",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsIndexInBounds(bool inBounds, const ExecutionContext& context)
{
	if (!inBounds)
	{
		throw VmException(
			"E_VM_OUT_OF_BOUNDS",
			"Выход за границы массива",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsNativeFunctionFound(bool isFound, const ExecutionContext& context)
{
	if (!isFound)
	{
		throw VmException(
			"E_VM_NATIVE_NOT_FOUND",
			"Вызов незарегистрированной нативной функции",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsStackNotEmpty(const ExecutionContext& context)
{
	if (context.stackTop == 0)
	{
		throw VmException(
			"E_VM_STACK_EMPTY",
			"Стек пуст, невозможно извлечь значение",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsStackNotFull(const ExecutionContext& context)
{
	if (context.stackTop >= context.stack.size())
	{
		throw VmException(
			"E_VM_STACK_OVERFLOW",
			"Переполнение стека виртуальной машины",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsIpValid(uint32_t targetIp, const ExecutionContext& context)
{
	if (targetIp >= context.chunk.GetCode().size())
	{
		throw VmException(
			"E_VM_IP_OUT_OF_BOUNDS",
			"Указатель инструкций вышел за пределы памяти",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsUnknownOpCode(bool isUnknown, const ExecutionContext& context)
{
	if (isUnknown)
	{
		throw VmException(
			"E_VM_UNKNOWN_OPCODE",
			"Обнаружен неизвестный код операции",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsStackDistanceValid(bool isValid, const ExecutionContext& context)
{
	if (!isValid)
	{
		throw VmException(
			"E_VM_STACK_OOB",
			"Попытка доступа к элементу за пределами стека",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsNotDivisionByZero(bool isZero, const ExecutionContext& context)
{
	if (isZero)
	{
		throw VmException(
			"E_VM_DIV_BY_ZERO",
			"Деление на ноль",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsCallStackOverflow(const ExecutionContext& context)
{
	if (context.frames.size() >= MAX_FRAMES)
	{
		throw VmException(
			"E_VM_CALL_STACK_OVERFLOW",
			"Превышен лимит вложенности вызовов функций",
			context.ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsCallArgumentCountValid(uint32_t argCount, const ExecutionContext& context)
{
	if (context.stackTop < argCount)
	{
		throw VmException(
			"E_VM_ARGS_COUNT",
			"Недостаточно аргументов на стеке для вызова функции",
			context.ip,
			ExtractCurrentLine(context));
	}
}

uint8_t ReadByte(ExecutionContext& context)
{
	AssertIsIpValid(context.ip, context);
	return context.chunk.GetCode()[context.ip++];
}

uint32_t ReadUint32(ExecutionContext& context)
{
	AssertIsIpValid(context.ip + 3, context);
	uint32_t value = 0;
	value |= static_cast<uint32_t>(context.chunk.GetCode()[context.ip++]);
	value |= static_cast<uint32_t>(context.chunk.GetCode()[context.ip++]) << 8;
	value |= static_cast<uint32_t>(context.chunk.GetCode()[context.ip++]) << 16;
	value |= static_cast<uint32_t>(context.chunk.GetCode()[context.ip++]) << 24;
	return value;
}

void Push(ExecutionContext& context, const Value& value)
{
	AssertIsStackNotFull(context);
	context.stack[context.stackTop++] = value;
}

Value Pop(ExecutionContext& context)
{
	AssertIsStackNotEmpty(context);
	return context.stack[--context.stackTop];
}

template <typename T>
void ExecuteAdd(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs + rhs)));
}

template <typename T>
void ExecuteSub(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs - rhs)));
}

template <typename T>
void ExecuteMul(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs * rhs)));
}

template <typename T>
void ExecuteDiv(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();

	AssertIsNotDivisionByZero(rhs == 0, context);

	if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
	{
		AssertIsNotDivisionOverflow(lhs == std::numeric_limits<T>::min() && rhs == -1, context);
	}

	Push(context, Value(static_cast<T>(lhs / rhs)));
}

template <typename T>
void ExecuteRem(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();

	AssertIsNotDivisionByZero(rhs == 0, context);

	if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
	{
		AssertIsNotDivisionOverflow(lhs == std::numeric_limits<T>::min() && rhs == -1, context);
	}

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
void ExecuteEq(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<uint64_t>(lhs == rhs ? 1 : 0)));
}

template <typename T>
void ExecuteLt(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<uint64_t>(lhs < rhs ? 1 : 0)));
}

void ExecuteConstantInstruction(ExecutionContext& context)
{
	const uint8_t index = ReadByte(context);
	const Value constant = context.chunk.GetConstants()[index];
	Push(context, constant);
}

void ExecuteDupInstruction(ExecutionContext& context)
{
	AssertIsStackNotEmpty(context);
	Push(context, context.stack[context.stackTop - 1]);
}

void ExecuteLoadLocalInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const uint32_t absoluteIndex = context.stackOffset + index;
	AssertIsStackDistanceValid(absoluteIndex < context.stackTop, context);
	Push(context, context.stack[absoluteIndex]);
}

void ExecuteStoreLocalInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const uint32_t absoluteIndex = context.stackOffset + index;
	AssertIsStackDistanceValid(absoluteIndex < context.stackTop, context);
	context.stack[absoluteIndex] = Pop(context);
}

void ExecuteJumpInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	context.ip = targetIp;
}

void ExecuteJumpIfFalseInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	const bool condition = Pop(context).As<bool>();

	if (!condition)
	{
		context.ip = targetIp;
	}
}

void ExecuteJumpIfTrueInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	const bool condition = Pop(context).As<bool>();

	if (condition)
	{
		context.ip = targetIp;
	}
}

void ExecuteCallInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	const uint32_t argCount = ReadUint32(context);

	AssertIsCallStackOverflow(context);
	AssertIsCallArgumentCountValid(argCount, context);

	context.frames.push_back({context.ip, context.stackOffset});

	context.ip = targetIp;
	context.stackOffset = context.stackTop - argCount;
}

bool ExecuteReturnInstruction(ExecutionContext& context)
{
	const Value result = Pop(context);

	if (context.frames.empty())
	{
		Push(context, result);
		return true;
	}

	context.stackTop = context.stackOffset;
	Push(context, result);

	const auto [returnIp, stackOffset] = context.frames.back();
	context.frames.pop_back();

	context.ip = returnIp;
	context.stackOffset = stackOffset;

	return false;
}

void ExecuteAllocateStructInstruction(ExecutionContext& context)
{
	const uint32_t fieldCount = ReadUint32(context);
	auto* object = new HeapObject(fieldCount);
	context.tracker.Track(object);
	Push(context, Value(reinterpret_cast<uint64_t>(object)));
}

void ExecuteGetFieldInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	Push(context, object->GetField(index));
}

void ExecuteStoreFieldInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const Value value = Pop(context);
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	object->SetField(index, value);
}

void ExecuteAllocateArrayInstruction(ExecutionContext& context)
{
	const uint32_t length = Pop(context).As<uint32_t>();
	auto* object = new HeapObject(length);
	context.tracker.Track(object);
	Push(context, Value(reinterpret_cast<uint64_t>(object)));
}

void ExecuteLoadElementInstruction(ExecutionContext& context)
{
	const uint32_t index = Pop(context).As<uint32_t>();
	const auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	AssertIsIndexInBounds(index < object->GetSize(), context);
	Push(context, object->GetField(index));
}

void ExecuteStoreElementInstruction(ExecutionContext& context)
{
	const Value value = Pop(context);
	const uint32_t index = Pop(context).As<uint32_t>();
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	AssertIsIndexInBounds(index < object->GetSize(), context);
	object->SetField(index, value);
}

void ExecuteRetainInstruction(ExecutionContext& context)
{
	const auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	Push(context, Value(object->GetSize()));
}

void ExecuteArrayLengthInstruction(ExecutionContext& context)
{
	const auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	Push(context, Value(object->GetSize()));
}

void ExecuteReleaseInstruction(ExecutionContext& context)
{
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	if (object->Release())
	{
		context.tracker.Untrack(object);
		delete object;
	}
}

void ExecuteAllocateClosureInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	const uint32_t captureCount = ReadUint32(context);

	auto* closure = new HeapObject(captureCount + 1);
	context.tracker.Track(closure);

	closure->SetField(0, Value(static_cast<uint64_t>(targetIp)));

	for (uint32_t i = captureCount; i > 0; --i)
	{
		closure->SetField(i, Pop(context));
	}

	Push(context, Value(reinterpret_cast<uint64_t>(closure)));
}

void ExecuteCallClosureInstruction(ExecutionContext& context)
{
	const uint32_t argCount = ReadUint32(context);

	AssertIsCallStackOverflow(context);
	AssertIsCallArgumentCountValid(argCount + 1, context);

	const uint32_t absoluteIndex = context.stackTop - argCount - 1;
	auto* closure = reinterpret_cast<HeapObject*>(context.stack[absoluteIndex].AsRaw());
	const uint32_t targetIp = static_cast<uint32_t>(closure->GetField(0).AsRaw());

	context.frames.push_back({context.ip, context.stackOffset});

	context.ip = targetIp;
	context.stackOffset = absoluteIndex;
}

void ExecuteCallNativeInstruction(ExecutionContext& context)
{
	const uint32_t nativeId = ReadUint32(context);
	const uint32_t argCount = ReadUint32(context);

	AssertIsCallArgumentCountValid(argCount, context);
	AssertIsNativeFunctionFound(nativeId < context.natives.size() && context.natives[nativeId], context);

	const uint32_t absoluteIndex = context.stackTop - argCount;
	const std::span<const Value> args(&context.stack[absoluteIndex], argCount);

	const Value result = context.natives[nativeId](args);

	context.stackTop -= argCount;
	Push(context, result);
}

void ExecuteLoadStringInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const std::string& text = context.chunk.GetStrings()[index];

	auto* stringObject = new HeapObject(1);
	context.tracker.Track(stringObject);

	stringObject->SetField(0, Value(reinterpret_cast<uint64_t>(&text)));

	Push(context, Value(reinterpret_cast<uint64_t>(stringObject)));
}

void ExecutePanicInstruction(ExecutionContext& context)
{
	const uint32_t errorCode = Pop(context).As<uint32_t>();

	throw VmException(
		"E_VM_RUNTIME_PANIC",
		"Критическая ошибка выполнения (Runtime Panic). Код: " + std::to_string(errorCode),
		context.ip,
		ExtractCurrentLine(context));
}

void ExecuteAddStringInstruction(ExecutionContext& context)
{
	auto* rhsObj = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	auto* lhsObj = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());

	AssertIsStringNotNull(rhsObj, context);
	AssertIsStringNotNull(lhsObj, context);

	const auto* rhsStr = reinterpret_cast<const std::string*>(rhsObj->GetField(0).AsRaw());
	const auto* lhsStr = reinterpret_cast<const std::string*>(lhsObj->GetField(0).AsRaw());

	auto newStr = std::make_unique<std::string>(*lhsStr + *rhsStr);
	const auto* rawPtr = newStr.get();
	context.dynamicStrings.push_back(std::move(newStr));

	auto* newObj = new HeapObject(1);
	context.tracker.Track(newObj);
	newObj->SetField(0, Value(reinterpret_cast<uint64_t>(rawPtr)));

	Push(context, Value(reinterpret_cast<uint64_t>(newObj)));
}

void ExecuteEqStringInstruction(ExecutionContext& context)
{
	auto* rhsObj = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	auto* lhsObj = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());

	AssertIsStringNotNull(rhsObj, context);
	AssertIsStringNotNull(lhsObj, context);

	const auto* rhsStr = reinterpret_cast<const std::string*>(rhsObj->GetField(0).AsRaw());
	const auto* lhsStr = reinterpret_cast<const std::string*>(lhsObj->GetField(0).AsRaw());

	const bool isEqual = (*lhsStr == *rhsStr);
	Push(context, Value(static_cast<uint64_t>(isEqual ? 1 : 0)));
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

		case OpCode::AddInt:
			ExecuteAdd<int64_t>(context);
			break;
		case OpCode::AddUint:
			ExecuteAdd<uint64_t>(context);
			break;
		case OpCode::AddReal:
			ExecuteAdd<double>(context);
			break;
		case OpCode::AddString:
			ExecuteAddStringInstruction(context);
			break;

		case OpCode::SubInt:
			ExecuteSub<int64_t>(context);
			break;
		case OpCode::SubUint:
			ExecuteSub<uint64_t>(context);
			break;
		case OpCode::SubReal:
			ExecuteSub<double>(context);
			break;

		case OpCode::MulInt:
			ExecuteMul<int64_t>(context);
			break;
		case OpCode::MulUint:
			ExecuteMul<uint64_t>(context);
			break;
		case OpCode::MulReal:
			ExecuteMul<double>(context);
			break;

		case OpCode::DivInt:
			ExecuteDiv<int64_t>(context);
			break;
		case OpCode::DivUint:
			ExecuteDiv<uint64_t>(context);
			break;
		case OpCode::DivReal:
			ExecuteDiv<double>(context);
			break;

		case OpCode::RemInt:
			ExecuteRem<int64_t>(context);
			break;
		case OpCode::RemUint:
			ExecuteRem<uint64_t>(context);
			break;
		case OpCode::RemReal:
			ExecuteRem<double>(context);
			break;

		case OpCode::EqInt:
			ExecuteEq<int64_t>(context);
			break;
		case OpCode::EqUint:
			ExecuteEq<uint64_t>(context);
			break;
		case OpCode::EqReal:
			ExecuteEq<double>(context);
			break;
		case OpCode::EqString:
			ExecuteEqStringInstruction(context);
			break;

		case OpCode::LtInt:
			ExecuteLt<int64_t>(context);
			break;
		case OpCode::LtUint:
			ExecuteLt<uint64_t>(context);
			break;
		case OpCode::LtReal:
			ExecuteLt<double>(context);
			break;

		case OpCode::LogicalNot: {
			const bool value = Pop(context).As<bool>();
			Push(context, Value(!value));
			break;
		}

		case OpCode::Pop:
			Pop(context);
			break;
		case OpCode::Dup:
			ExecuteDupInstruction(context);
			break;

		case OpCode::LoadLocal:
			ExecuteLoadLocalInstruction(context);
			break;
		case OpCode::StoreLocal:
			ExecuteStoreLocalInstruction(context);
			break;
		case OpCode::Jump:
			ExecuteJumpInstruction(context);
			break;
		case OpCode::JumpIfFalse:
			ExecuteJumpIfFalseInstruction(context);
			break;
		case OpCode::JumpIfTrue:
			ExecuteJumpIfTrueInstruction(context);
			break;

		case OpCode::AllocateStruct:
			ExecuteAllocateStructInstruction(context);
			break;
		case OpCode::GetField:
			ExecuteGetFieldInstruction(context);
			break;
		case OpCode::StoreField:
			ExecuteStoreFieldInstruction(context);
			break;

		case OpCode::AllocateArray:
			ExecuteAllocateArrayInstruction(context);
			break;
		case OpCode::LoadElement:
			ExecuteLoadElementInstruction(context);
			break;
		case OpCode::StoreElement:
			ExecuteStoreElementInstruction(context);
			break;
		case OpCode::ArrayLength:
			ExecuteArrayLengthInstruction(context);
		case OpCode::Retain:
			ExecuteRetainInstruction(context);
			break;
		case OpCode::Release:
			ExecuteReleaseInstruction(context);
			break;

		case OpCode::Call:
			ExecuteCallInstruction(context);
			break;

		case OpCode::AllocateClosure:
			ExecuteAllocateClosureInstruction(context);
			break;
		case OpCode::CallClosure:
			ExecuteCallClosureInstruction(context);
			break;

		case OpCode::CallNative:
			ExecuteCallNativeInstruction(context);
			break;

		case OpCode::LoadString:
			ExecuteLoadStringInstruction(context);
			break;

		case OpCode::Panic:
			ExecutePanicInstruction(context);
			break;

		case OpCode::Return:
			if (ExecuteReturnInstruction(context))
			{
				return;
			}
			break;

		default:
			AssertIsUnknownOpCode(true, context);
		}
	}
}
} // namespace

VirtualMachine::VirtualMachine()
	: m_stackTop(0)
{
	m_stack.resize(MAX_STACK);
	m_frames.reserve(MAX_FRAMES);
}

void VirtualMachine::RegisterNativeFunction(uint32_t id, NativeCallback callback)
{
	if (id >= m_natives.size())
	{
		m_natives.resize(id + 1);
	}
	m_natives[id] = std::move(callback);
}

void VirtualMachine::Interpret(const Chunk& chunk)
{
	m_stackTop = 0;
	m_frames.clear();
	m_tracker.Clear();

	ExecutionContext context{chunk, m_stack, m_frames, m_natives, m_tracker, m_stackTop, 0, 0, m_dynamicStrings};
	Run(context);
}

Value VirtualMachine::Peek(size_t distance) const
{
	if (distance >= m_stackTop)
	{
		throw std::runtime_error("Попытка доступа к элементу за пределами стека через публичный API");
	}

	return m_stack[m_stackTop - 1 - distance];
}