#pragma once
#include "Diagnostic.h"
#include <vector>

class DiagnosticEngine
{
public:
	void Report(DiagnosticData data);

	void Clear();
	bool HasErrors() const;
	const std::vector<DiagnosticData>& GetInfo() const;

private:
	std::vector<DiagnosticData> m_diagnostics;
};