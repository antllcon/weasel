#include "BackendException.h"
#include <utility>

BackendException::BackendException(CompilerPhase phase, std::string errorCode, std::string message)
	: std::runtime_error(message)
	, m_phase(phase)
	, m_errorCode(std::move(errorCode))
	, m_message(std::move(message))
{
}

CompilerPhase BackendException::GetPhase() const noexcept
{
	return m_phase;
}

const std::string& BackendException::GetErrorCode() const noexcept
{
	return m_errorCode;
}

const std::string& BackendException::GetErrorMessage() const noexcept
{
	return m_message;
}