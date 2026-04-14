#include "CompilationException.h"

#include <sstream>
#include <utility>

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

void AppendReason(std::ostringstream& ss, const DiagnosticData& data)
{
	ss << "  -> Причина: " << data.message << "\n";
}

void AppendExpectedActual(std::ostringstream& ss, const DiagnosticData& data)
{
	if (data.expected.empty() && data.actual.empty())
	{
		return;
	}

	ss << "  -> Ожидалось: " << (data.expected.empty() ? "нет данных" : data.expected) << "\n";
	ss << "  -> Получено: " << (data.actual.empty() ? "нет данных" : data.actual) << "\n";
}

void AppendLocation(std::ostringstream& ss, const DiagnosticData& data)
{
	if (data.line == 0)
	{
		return;
	}

	ss << "  -> Локация: Строка " << data.line << ", Позиция " << data.pos;

	if (!data.filePath.empty())
	{
		ss << " (Файл: " << data.filePath.string() << ")";
	}

	ss << "\n";
}

void AppendDocumentation(std::ostringstream& ss, const DiagnosticData& data)
{
	ss << "  -> Документация: https://weasel-lang.org/docs/errors#" << data.errorCode << "\n";
}

std::string FormatErrorMessage(const DiagnosticData& data)
{
	std::ostringstream ss;

	ss << "\n[" << PhaseToString(data.phase) << "] Ошибка " << data.errorCode << ":\n";
	AppendReason(ss, data);
	AppendExpectedActual(ss, data);
	AppendLocation(ss, data);
	AppendDocumentation(ss, data);

	return ss.str();
}
} // namespace

CompilationException::CompilationException(DiagnosticData data)
	: std::runtime_error(FormatErrorMessage(data))
	, m_data(std::move(data))
{
}

const DiagnosticData& CompilationException::GetData() const noexcept
{
	return m_data;
}