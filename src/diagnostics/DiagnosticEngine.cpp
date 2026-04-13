#include "DiagnosticEngine.h"

void DiagnosticEngine::Report(DiagnosticData data)
{
	m_diagnostics.emplace_back(std::move(data));
}

void DiagnosticEngine::Clear()
{
	m_diagnostics.clear();
}

bool DiagnosticEngine::HasErrors() const
{
	return !m_diagnostics.empty();
}

const std::vector<DiagnosticData>& DiagnosticEngine::GetInfo() const
{
	return m_diagnostics;
}