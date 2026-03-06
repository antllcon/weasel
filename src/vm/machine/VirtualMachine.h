#pragma once
#include "src/vm/chank/Chunk.h"
#include "src/vm/value/Value.h"

#include <vector>

class VirtualMachine
{
public:
	VirtualMachine();
	~VirtualMachine();

	void Interpret(const Chunk& chunk);
	Value GetLastResult() const;

private:
	std::vector<Value> m_stack;
	Value m_lastResult;
};