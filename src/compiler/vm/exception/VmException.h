#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

class VmException final : public std::runtime_error
{
public:
	VmException(std::string errorCode, std::string message, uint32_t instructionPointer, uint32_t line);

	[[nodiscard]] const std::string& GetErrorCode() const noexcept;
	[[nodiscard]] const std::string& GetErrorMessage() const noexcept;
	[[nodiscard]] uint32_t GetInstructionPointer() const noexcept;
	[[nodiscard]] uint32_t GetLine() const noexcept;

private:
	std::string m_errorCode;
	std::string m_message;
	uint32_t m_instructionPointer;
	uint32_t m_line;
};