#pragma once
#include "src/vm/chank/Chunk.h"
#include "src/vm/value/Value.h"
#include <vector>

class VirtualMachine
{
public:
	VirtualMachine();

	void Interpret(const Chunk& chunk);
	Value Peek(size_t distance = 0) const;

private:
	std::vector<Value> m_stack;
};