#pragma once
#include "src/vm/chunk/Chunk.h"
#include "src/vm/memory/HeapTracker.h"
#include "src/vm/value/Value.h"
#include <vector>

class VirtualMachine
{
public:
	struct CallFrame
	{
		uint32_t m_returnIp;
		uint32_t m_stackOffset;
	};

	VirtualMachine();

	void Interpret(const Chunk& chunk);
	Value Peek(size_t distance = 0) const;

private:
	std::vector<Value> m_stack;
	std::vector<CallFrame> m_frames;
	HeapTracker m_tracker;
	uint32_t m_stackTop;
};