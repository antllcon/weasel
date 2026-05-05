#pragma once
#include "src/compiler/vm/value/Value.h"
#include "src/compiler/vm/OpCode.h"
#include <cstdint>
#include <string>
#include <vector>

class Chunk
{
public:
	void WriteByte(uint8_t byte, uint32_t line);
	void WriteUint32(uint32_t value, uint32_t line);
	void WriteOpCode(OpCode code, uint32_t line);
	uint8_t AddConstant(Value value);
	uint8_t AddString(const std::string& text);

	[[nodiscard]] uint32_t GetCodeSize() const;
	void PatchUint32(uint32_t offset, uint32_t value);

	const std::vector<uint8_t>& GetCode() const;
	const std::vector<Value>& GetConstants() const;
	const std::vector<std::string>& GetStrings() const;
	uint32_t GetLine(uint32_t instructionOffset) const;

private:
	std::vector<uint8_t> m_code;
	std::vector<uint32_t> m_lines;
	std::vector<Value> m_constants;
	std::vector<std::string> m_strings;
};