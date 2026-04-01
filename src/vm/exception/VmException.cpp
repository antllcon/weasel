#include "VmException.h"
#include <utility>

VmException::VmException(std::string errorCode, std::string message, uint32_t instructionPointer, uint32_t line)
	: std::runtime_error(message)
	, m_errorCode(std::move(errorCode))
	, m_message(std::move(message))
	, m_instructionPointer(instructionPointer)
	, m_line(line)
{
}

const std::string& VmException::GetErrorCode() const noexcept
{
	return m_errorCode;
}

const std::string& VmException::GetErrorMessage() const noexcept
{
	return m_message;
}

uint32_t VmException::GetInstructionPointer() const noexcept
{
	return m_instructionPointer;
}

uint32_t VmException::GetLine() const noexcept
{
	return m_line;
}