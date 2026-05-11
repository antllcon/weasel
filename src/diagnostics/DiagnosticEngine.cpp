#include "DiagnosticEngine.h"
#include <sstream>

namespace
{
std::string PhaseToString(CompilerPhase phase)
{
	switch (phase)
	{
	case CompilerPhase::Lexer:
		return "Lexer";
	case CompilerPhase::Parser:
		return "Parser";
	case CompilerPhase::Semantic:
		return "Semantic";
	case CompilerPhase::Optimizer:
		return "Optimizer";
	case CompilerPhase::Backend:
		return "Backend";
	case CompilerPhase::VirtualMachine:
		return "virtual";
	}
	return "anon-phase";
}
} // namespace

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

const std::vector<DiagnosticData>& DiagnosticEngine::GetDiagnostics() const
{
	return m_diagnostics;
}

std::string DiagnosticEngine::FormatMessage(const DiagnosticData& data)
{
	std::ostringstream ss;
	ss << "[" << PhaseToString(data.phase) << "]\t" << data.message;

	if (!data.expected.empty() || !data.actual.empty())
	{
		ss << "\n  -> Ожидалось: " << (data.expected.empty() ? "нет данных" : data.expected);
		ss << "\n  -> Получено:  " << (data.actual.empty() ? "нет данных" : data.actual);
	}

	if (data.line != 0)
	{
		ss << "\n  -> Строка " << data.line << ", позиция " << data.pos;
		if (!data.filePath.empty())
		{
			ss << " (" << data.filePath.string() << ")";
		}
	}

	return ss.str();
}