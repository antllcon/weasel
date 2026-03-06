#pragma once
#include "src/vm/OpCode.h"
#include "src/vm/value/Value.h"
#include <cstdint>
#include <vector>

class Chunk
{
public:
	void WriteBite(uint8_t byte);
	void WriteOpCode(OpCode code);
	uint8_t AddConstant(Value value);

	const std::vector<uint8_t>& GetCode() const;
	const std::vector<Value>& GetConstants() const;

private:
	std::vector<uint8_t> m_code;
	std::vector<Value> m_constants;
};