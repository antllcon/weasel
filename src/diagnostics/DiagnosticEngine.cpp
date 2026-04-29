#include "DiagnosticEngine.h"
#include <sstream>

namespace
{
std::string PhaseToString(CompilerPhase phase)
{
	switch (phase)
	{
	case CompilerPhase::Lexer:
		return "Лексический анализ";
	case CompilerPhase::Parser:
		return "Синтаксический анализ";
	case CompilerPhase::Semantic:
		return "Семантический анализ";
	case CompilerPhase::Optimizer:
		return "Оптимизация";
	case CompilerPhase::Backend:
		return "Генерация кода";
	case CompilerPhase::VirtualMachine:
		return "Среда выполнения (VM)";
	}
	return "Неизвестная фаза";
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
	ss << "[" << PhaseToString(data.phase) << "] Ошибка " << data.errorCode << ": " << data.message;

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