#include "LalrParseStepsPrinter.h"
#include "src/utils/logger/Logger.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace
{
constexpr size_t StepWidth = 6;
constexpr size_t StackWidth = 22;
constexpr size_t InputWidth = 22;
constexpr size_t SeparatorLength = 98;

std::string Truncate(const std::string& str, size_t width)
{
	if (str.length() > width)
	{
		return str.substr(0, width - 3) + "...";
	}
	return str;
}

std::string FormatSeparator()
{
	return std::string(SeparatorLength, '-');
}

std::string FormatHeader(const std::string&)
{
	std::ostringstream ss;
	ss << "Процесс восходящего разбора (LALR-1) для программы";
	return ss.str();
}

std::string FormatColumnsHeader()
{
	std::ostringstream ss;
	ss << std::left
	   << std::setw(StepWidth) << "Step"
	   << std::setw(StackWidth) << "State Stack"
	   << std::setw(StackWidth) << "Symbol Stack"
	   << std::setw(InputWidth) << "Input"
	   << "| Action";
	return ss.str();
}

std::string FormatStepRow(const LalrParseStep& step)
{
	std::ostringstream ss;
	ss << std::left
	   << std::setw(StepWidth) << step.stepNumber
	   << std::setw(StackWidth) << Truncate(step.stateStack, StackWidth - 2)
	   << std::setw(StackWidth) << Truncate(step.symbolStack, StackWidth - 2)
	   << std::setw(InputWidth) << Truncate(step.input, InputWidth - 2)
	   << "| -> " << step.action;
	return ss.str();
}
} // namespace

namespace LalrParseStepsPrinter
{
void Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine)
{
	Logger::Log("[Parser]\t" + FormatHeader(inputLine));
	Logger::Log("\t\t" + FormatSeparator());
	Logger::Log("\t\t" + FormatColumnsHeader());
	Logger::Log("\t\t" + FormatSeparator());

	for (const auto& step : steps)
	{
		Logger::Log("\t\t" + FormatStepRow(step));
	}

	Logger::Log("\t\t" + FormatSeparator());
}
} // namespace LalrParseStepsPrinter