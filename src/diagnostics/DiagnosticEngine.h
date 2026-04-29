#pragma once
#include "Diagnostic.h"
#include <string>
#include <vector>

class DiagnosticEngine
{
public:
	void Report(DiagnosticData data);

	void Clear();
	[[nodiscard]] bool HasErrors() const;
	[[nodiscard]] const std::vector<DiagnosticData>& GetDiagnostics() const;

	[[nodiscard]] static std::string FormatMessage(const DiagnosticData& data);

private:
	std::vector<DiagnosticData> m_diagnostics;
};