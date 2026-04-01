#pragma once
#include "Diagnostic.h"
#include <stdexcept>

class CompilationException : public std::runtime_error
{
public:
	explicit CompilationException(const DiagnosticData& data);

	[[nodiscard]] const DiagnosticData& GetData() const noexcept;

private:
	DiagnosticData m_data;
};