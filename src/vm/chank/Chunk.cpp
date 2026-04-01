#include "Chunk.h"
#include <stdexcept>

namespace
{
void AssertIsConstantIndexValid(size_t size)
{
	if (size > 255)
	{
		throw std::runtime_error("Превышен лимит констант в одном блоке (максимум 256)");
	}
}

void AssertIsStringIndexValid(size_t size)
{
	if (size > 255)
	{
		throw std::runtime_error("Превышен лимит строк в одном блоке (максимум 256)");
	}
}

void AssertIsOffsetValid(bool isValid)
{
	if (!isValid)
	{
		throw std::runtime_error("Смещение инструкции выходит за пределы массива строк кода");
	}
}
}

void Chunk::WriteByte(uint8_t byte, uint32_t line)
{
	m_code.push_back(byte);
	m_lines.push_back(line);
}

void Chunk::WriteUint32(uint32_t value, uint32_t line)
{
	WriteByte(static_cast<uint8_t>(value & 0xFF), line);
	WriteByte(static_cast<uint8_t>(value >> 8 & 0xFF), line);
	WriteByte(static_cast<uint8_t>(value >> 16 & 0xFF), line);
	WriteByte(static_cast<uint8_t>(value >> 24 & 0xFF), line);
}

void Chunk::WriteOpCode(OpCode code, uint32_t line)
{
	m_code.push_back(static_cast<uint8_t>(code));
	m_lines.push_back(line);
}

uint8_t Chunk::AddConstant(Value value)
{
	AssertIsConstantIndexValid(m_constants.size());
	m_constants.push_back(value);
	return static_cast<uint8_t>(m_constants.size() - 1);
}

uint8_t Chunk::AddString(const std::string& text)
{
	AssertIsStringIndexValid(m_strings.size());
	m_strings.push_back(text);
	return static_cast<uint8_t>(m_strings.size() - 1);
}

const std::vector<uint8_t>& Chunk::GetCode() const
{
	return m_code;
}

const std::vector<Value>& Chunk::GetConstants() const
{
	return m_constants;
}

const std::vector<std::string>& Chunk::GetStrings() const
{
	return m_strings;
}

uint32_t Chunk::GetLine(uint32_t instructionOffset) const
{
	AssertIsOffsetValid(instructionOffset < m_lines.size());
	return m_lines[instructionOffset];
}