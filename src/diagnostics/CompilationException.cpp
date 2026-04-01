#include "CompilationException.h"
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
	default:
		return "Неизвестная фаза";
	}
}

std::string BuildErrorMessage(const DiagnosticData& data)
{
	std::ostringstream ss;

	ss << "\n[" << PhaseToString(data.phase) << "] Ошибка " << data.errorCode << ":\n";
	ss << "  -> Причина: " << data.message << "\n";

	if (!data.expected.empty() || !data.actual.empty())
	{
		ss << "  -> Ожидалось: " << (data.expected.empty() ? "нет данных" : data.expected) << "\n";
		ss << "  -> Получено: " << (data.actual.empty() ? "нет данных" : data.actual) << "\n";
	}

	if (data.line > 0)
	{
		ss << "  -> Локация: Строка " << data.line << ", Позиция " << data.pos;
		if (!data.filePath.empty())
		{
			ss << " (Файл: " << data.filePath << ")";
		}
		ss << "\n";
	}

	ss << "  -> Документация: https://weasel-lang.org/docs/errors#" << data.errorCode << "\n";

	return ss.str();
}
} // namespace

CompilationException::CompilationException(const DiagnosticData& data)
	: std::runtime_error(BuildErrorMessage(data))
	, m_data(data)
{
}

const DiagnosticData& CompilationException::GetData() const noexcept
{
	return m_data;
}