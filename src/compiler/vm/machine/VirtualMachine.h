#pragma once

#include "src/compiler/stdlib/NativeTypes.h"
#include "src/compiler/vm/memory/HeapTracker.h"
#include "src/compiler/vm/value/Value.h"
#include <memory>
#include <string>
#include <vector>

class Chunk;

class VirtualMachine
{
public:
	struct CallFrame
	{
		uint32_t returnIp;
		uint32_t stackOffset;
	};

	VirtualMachine();

	void RegisterNativeFunction(uint32_t id, NativeCallback callback);
	void Interpret(const Chunk& chunk);
	[[nodiscard]] Value Peek(size_t distance = 0) const;

private:
	std::vector<Value> m_stack;
	std::vector<CallFrame> m_frames;
	std::vector<NativeCallback> m_natives;
	HeapTracker m_tracker;
	uint32_t m_stackTop;
	std::vector<std::unique_ptr<std::string>> m_dynamicStrings;
};