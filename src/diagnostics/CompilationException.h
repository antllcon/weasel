#pragma once
#include "Diagnostic.h"
#include <stdexcept>

class CompilationException final : public std::runtime_error
{
public:
	explicit CompilationException(DiagnosticData data);
	const DiagnosticData& GetData() const noexcept;

private:
	DiagnosticData m_data;
};