#pragma once
#include "src/diagnostics/Diagnostic.h"
#include <stdexcept>
#include <string>

class BackendException final : public std::runtime_error
{
public:
	BackendException(CompilerPhase phase, std::string errorCode, std::string message);

	[[nodiscard]] CompilerPhase GetPhase() const noexcept;
	[[nodiscard]] const std::string& GetErrorCode() const noexcept;
	[[nodiscard]] const std::string& GetErrorMessage() const noexcept;

private:
	CompilerPhase m_phase;
	std::string m_errorCode;
	std::string m_message;
};