#include "VirtualMachine.h"
#include "../exception/VmException.h"
#include "src/vm/memory/HeapObject.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace
{
inline constexpr uint32_t MAX_STACK = 65536u;
inline constexpr uint16_t MAX_FRAMES = 256u;

struct ExecutionContext
{
	const Chunk& m_chunk;
	std::vector<Value>& m_stack;
	std::vector<VirtualMachine::CallFrame>& m_frames;
	const std::vector<VirtualMachine::NativeCallback>& m_natives;
	HeapTracker& m_tracker;
	uint32_t& m_stackTop;
	uint32_t m_ip;
	uint32_t m_stackOffset;
};

uint32_t ExtractCurrentLine(const ExecutionContext& context)
{
	const uint32_t errorIp = context.m_ip > 0 ? context.m_ip - 1 : 0;
	return context.m_chunk.GetLine(errorIp);
}

void AssertIsNativeFunctionFound(bool isFound, const ExecutionContext& context)
{
	if (!isFound)
	{
		throw VmException(
			"E_VM_NATIVE_NOT_FOUND",
			"Вызов незарегистрированной нативной функции",
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsStackNotEmpty(const ExecutionContext& context)
{
	if (context.m_stackTop == 0)
	{
		throw VmException(
			"E_VM_STACK_EMPTY",
			"Стек пуст, невозможно извлечь значение",
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsStackNotFull(const ExecutionContext& context)
{
	if (context.m_stackTop >= context.m_stack.size())
	{
		throw VmException(
			"E_VM_STACK_OVERFLOW",
			"Переполнение стека виртуальной машины",
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsIpValid(uint32_t targetIp, const ExecutionContext& context)
{
	if (targetIp >= context.m_chunk.GetCode().size())
	{
		throw VmException(
			"E_VM_IP_OUT_OF_BOUNDS",
			"Указатель инструкций вышел за пределы памяти",
			context.m_ip,
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
			context.m_ip,
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
			context.m_ip,
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
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsCallStackOverflow(const ExecutionContext& context)
{
	if (context.m_frames.size() >= MAX_FRAMES)
	{
		throw VmException(
			"E_VM_CALL_STACK_OVERFLOW",
			"Превышен лимит вложенности вызовов функций",
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

void AssertIsCallArgumentCountValid(uint32_t argCount, const ExecutionContext& context)
{
	if (context.m_stackTop < argCount)
	{
		throw VmException(
			"E_VM_ARGS_COUNT",
			"Недостаточно аргументов на стеке для вызова функции",
			context.m_ip,
			ExtractCurrentLine(context));
	}
}

uint8_t ReadByte(ExecutionContext& context)
{
	AssertIsIpValid(context.m_ip, context);
	return context.m_chunk.GetCode()[context.m_ip++];
}

uint32_t ReadUint32(ExecutionContext& context)
{
	AssertIsIpValid(context.m_ip + 3, context);
	uint32_t value = 0;
	value |= static_cast<uint32_t>(context.m_chunk.GetCode()[context.m_ip++]);
	value |= static_cast<uint32_t>(context.m_chunk.GetCode()[context.m_ip++]) << 8;
	value |= static_cast<uint32_t>(context.m_chunk.GetCode()[context.m_ip++]) << 16;
	value |= static_cast<uint32_t>(context.m_chunk.GetCode()[context.m_ip++]) << 24;
	return value;
}

void Push(ExecutionContext& context, const Value& value)
{
	AssertIsStackNotFull(context);
	context.m_stack[context.m_stackTop++] = value;
}

Value Pop(ExecutionContext& context)
{
	AssertIsStackNotEmpty(context);
	return context.m_stack[--context.m_stackTop];
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

	AssertIsNotDivisionByZero(rhs, context);

	if (rhs != 0)
	{
		Push(context, Value(static_cast<T>(lhs / rhs)));
	}
}

template <typename T>
void ExecuteRem(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();

	AssertIsNotDivisionByZero(rhs, context);

	if (rhs != 0)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			Push(context, Value(static_cast<T>(std::fmod(lhs, rhs))));
		}
		else
		{
			Push(context, Value(static_cast<T>(lhs % rhs)));
		}
	}
}

template <typename T>
void ExecuteEq(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(lhs == rhs));
}

template <typename T>
void ExecuteLt(ExecutionContext& context)
{
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(lhs < rhs));
}

template <typename T>
void ExecuteBitAnd(ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs & rhs)));
}

template <typename T>
void ExecuteBitOr(ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs | rhs)));
}

template <typename T>
void ExecuteBitXor(ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs ^ rhs)));
}

template <typename T>
void ExecuteBitNot(ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T operand = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(~operand)));
}

template <typename T>
void ExecuteShl(ExecutionContext& context)
{
	static_assert(std::is_integral_v<T>);
	const T rhs = Pop(context).As<T>();
	const T lhs = Pop(context).As<T>();
	Push(context, Value(static_cast<T>(lhs << rhs)));
}

template <typename T>
void ExecuteShr(ExecutionContext& context)
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
	AssertIsStackNotEmpty(context);
	Push(context, context.m_stack[context.m_stackTop - 1]);
}

void ExecuteLoadLocalInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const uint32_t absoluteIndex = context.m_stackOffset + index;
	AssertIsStackDistanceValid(absoluteIndex < context.m_stackTop, context);
	Push(context, context.m_stack[absoluteIndex]);
}

void ExecuteStoreLocalInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const uint32_t absoluteIndex = context.m_stackOffset + index;
	AssertIsStackDistanceValid(absoluteIndex < context.m_stackTop, context);
	context.m_stack[absoluteIndex] = Pop(context);
}

void ExecuteJumpInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	context.m_ip = targetIp;
}

void ExecuteJumpIfFalseInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	const bool condition = Pop(context).As<bool>();

	if (!condition)
	{
		context.m_ip = targetIp;
	}
}

void ExecuteJumpIfTrueInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	AssertIsIpValid(targetIp, context);
	const bool condition = Pop(context).As<bool>();

	if (condition)
	{
		context.m_ip = targetIp;
	}
}

void ExecuteCallInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	const uint32_t argCount = ReadUint32(context);

	AssertIsCallStackOverflow(context);
	AssertIsCallArgumentCountValid(argCount, context);

	context.m_frames.push_back({context.m_ip, context.m_stackOffset});

	context.m_ip = targetIp;
	context.m_stackOffset = context.m_stackTop - argCount;
}

bool ExecuteReturnInstruction(ExecutionContext& context)
{
	const Value result = Pop(context);

	if (context.m_frames.empty())
	{
		Push(context, result);
		return true;
	}

	context.m_stackTop = context.m_stackOffset;
	Push(context, result);

	const auto [m_returnIp, m_stackOffset] = context.m_frames.back();
	context.m_frames.pop_back();

	context.m_ip = m_returnIp;
	context.m_stackOffset = m_stackOffset;

	return false;
}

void ExecuteAllocateStructInstruction(ExecutionContext& context)
{
	const uint32_t fieldCount = ReadUint32(context);
	auto* object = new HeapObject(fieldCount);
	context.m_tracker.Track(object);
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
	context.m_tracker.Track(object);
	Push(context, Value(reinterpret_cast<uint64_t>(object)));
}

void ExecuteLoadElementInstruction(ExecutionContext& context)
{
	const uint32_t index = Pop(context).As<uint32_t>();
	const auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	Push(context, object->GetField(index));
}

void ExecuteStoreElementInstruction(ExecutionContext& context)
{
	const Value value = Pop(context);
	const uint32_t index = Pop(context).As<uint32_t>();
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	object->SetField(index, value);
}

void ExecuteRetainInstruction(ExecutionContext& context)
{
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	object->Retain();
	Push(context, Value(reinterpret_cast<uint64_t>(object)));
}

void ExecuteReleaseInstruction(ExecutionContext& context)
{
	auto* object = reinterpret_cast<HeapObject*>(Pop(context).AsRaw());
	if (object->Release())
	{
		context.m_tracker.Untrack(object);
		delete object;
	}
}

void ExecuteAllocateClosureInstruction(ExecutionContext& context)
{
	const uint32_t targetIp = ReadUint32(context);
	const uint32_t captureCount = ReadUint32(context);

	auto* closure = new HeapObject(captureCount + 1);
	context.m_tracker.Track(closure);

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

	const uint32_t absoluteIndex = context.m_stackTop - argCount - 1;
	auto* closure = reinterpret_cast<HeapObject*>(context.m_stack[absoluteIndex].AsRaw());
	const uint32_t targetIp = static_cast<uint32_t>(closure->GetField(0).AsRaw());

	context.m_frames.push_back({context.m_ip, context.m_stackOffset});

	context.m_ip = targetIp;
	context.m_stackOffset = absoluteIndex;
}

void ExecuteCallNativeInstruction(ExecutionContext& context)
{
	const uint32_t nativeId = ReadUint32(context);
	const uint32_t argCount = ReadUint32(context);

	AssertIsCallArgumentCountValid(argCount, context);
	AssertIsNativeFunctionFound(nativeId < context.m_natives.size() && context.m_natives[nativeId], context);

	const uint32_t absoluteIndex = context.m_stackTop - argCount;
	const std::span<const Value> args(&context.m_stack[absoluteIndex], argCount);

	const Value result = context.m_natives[nativeId](args);

	context.m_stackTop -= argCount;
	Push(context, result);
}

void ExecuteLoadStringInstruction(ExecutionContext& context)
{
	const uint32_t index = ReadUint32(context);
	const std::string& text = context.m_chunk.GetStrings()[index];

	// TODO: Для простоты пока создаем структуру, где в первом поле лежит указатель на строку
	// В будущем мы сделаем HeapObject более универсальным для строк и буферов
	auto* stringObject = new HeapObject(1);
	context.m_tracker.Track(stringObject);

	stringObject->SetField(0, Value(reinterpret_cast<uint64_t>(&text)));

	Push(context, Value(reinterpret_cast<uint64_t>(stringObject)));
}

void ExecutePanicInstruction(ExecutionContext& context)
{
	const uint32_t errorCode = Pop(context).As<uint32_t>();

	throw VmException(
		"E_VM_RUNTIME_PANIC",
		"Критическая ошибка выполнения (Runtime Panic). Код: " + std::to_string(errorCode),
		context.m_ip,
		ExtractCurrentLine(context));
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

		case OpCode::RemI8:
			ExecuteRem<int8_t>(context);
			break;
		case OpCode::RemU8:
			ExecuteRem<uint8_t>(context);
			break;
		case OpCode::RemI16:
			ExecuteRem<int16_t>(context);
			break;
		case OpCode::RemU16:
			ExecuteRem<uint16_t>(context);
			break;
		case OpCode::RemI32:
			ExecuteRem<int32_t>(context);
			break;
		case OpCode::RemU32:
			ExecuteRem<uint32_t>(context);
			break;
		case OpCode::RemI64:
			ExecuteRem<int64_t>(context);
			break;
		case OpCode::RemU64:
			ExecuteRem<uint64_t>(context);
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

	ExecutionContext context{chunk, m_stack, m_frames, m_natives, m_tracker, m_stackTop, 0, 0};

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