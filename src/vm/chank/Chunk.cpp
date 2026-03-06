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
} // namespace

void Chunk::WriteByte(uint8_t byte)
{
	m_code.push_back(byte);
}

void Chunk::WriteOpCode(OpCode code)
{
	m_code.push_back(static_cast<uint8_t>(code));
}

uint8_t Chunk::AddConstant(Value value)
{
	AssertIsConstantIndexValid(m_constants.size());
	m_constants.push_back(std::move(value));
	return static_cast<uint8_t>(m_constants.size() - 1);
}

const std::vector<uint8_t>& Chunk::GetCode() const
{
	return m_code;
}

const std::vector<Value>& Chunk::GetConstants() const
{
	return m_constants;
}