#include "Chunk.h"

#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/Diagnostic.h"

namespace
{
void AssertIsConstantIndexValid(size_t size)
{
	if (size > 255)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.errorCode = "хз-хз, дописать",
			.message = "Превышен лимит констант в одном блоке (максимум 256)"});
	}
}

void AssertIsStringIndexValid(size_t size)
{
	if (size > 255)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.errorCode = "хз-хз, дописать",
			.message = "Превышен лимит строк в одном блоке (максимум 256)"});
	}
}

void AssertIsOffsetValid(bool isValid)
{
	if (!isValid)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.errorCode = "хз-хз, дописать",
			.message = "Смещение инструкции выходит за пределы массива строк кода"});
	}
}

void AssertIsPatchOffsetValid(bool isValid)
{
	if (!isValid)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.errorCode = "хз-хз, дописать",
			.message = "Смещение для патча выходит за пределы байткода"});
	}
}
} // namespace

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

uint32_t Chunk::GetCodeSize() const
{
	return static_cast<uint32_t>(m_code.size());
}

void Chunk::PatchUint32(uint32_t offset, uint32_t value)
{
	AssertIsPatchOffsetValid(offset + 3 < m_code.size());
	m_code[offset + 0] = static_cast<uint8_t>(value & 0xFF);
	m_code[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
	m_code[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
	m_code[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
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