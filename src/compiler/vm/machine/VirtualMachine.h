#pragma once
#include "src/compiler/vm/value/Value.h"
#include "src/compiler/vm/memory/HeapTracker.h"
#include <functional>
#include <span>
#include <vector>

class Chunk;
class VirtualMachine
{
public:
	using NativeCallback = std::function<Value(std::span<const Value>)>;

	struct CallFrame
	{
		uint32_t m_returnIp;
		uint32_t m_stackOffset;
	};

	VirtualMachine();

	void RegisterNativeFunction(uint32_t id, NativeCallback callback);
	void Interpret(const Chunk& chunk);
	Value Peek(size_t distance = 0) const;

private:
	std::vector<Value> m_stack;
	std::vector<CallFrame> m_frames;
	std::vector<NativeCallback> m_natives;
	HeapTracker m_tracker;
	uint32_t m_stackTop;
};