#include "CompilationException.h"
#include "DiagnosticEngine.h"
#include <utility>

CompilationException::CompilationException(DiagnosticData data)
	: std::runtime_error(DiagnosticEngine::FormatMessage(data))
	, m_data(std::move(data))
{
}

const DiagnosticData& CompilationException::GetData() const noexcept
{
	return m_data;
}