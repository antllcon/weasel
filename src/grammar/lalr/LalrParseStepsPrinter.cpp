#include "LalrParseStepsPrinter.h"
#include "src/utils/logger/Logger.h"

#include <iomanip>
#include <sstream>

void LalrParseStepsPrinter::Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine)
{
	std::ostringstream ss;

	ss << "Процесс восходящего разбора (LALR-1) для: " << inputLine << "\n";
	ss << std::string(129, '-') << "\n";
	ss << std::left << std::setw(8) << "Step"
	   << std::setw(40) << "State Stack"
	   << std::setw(40) << "Symbol Stack"
	   << std::setw(40) << "Input"
	   << "| Action\n";
	ss << std::string(129, '-') << "\n";

	for (const auto& step : steps)
	{
		ss << std::left << std::setw(8) << step.stepNumber
		   << std::setw(40) << step.stateStack
		   << std::setw(40) << step.symbolStack
		   << std::setw(40) << step.input
		   << "| -> " << step.action << "\n";
	}

	ss << std::string(129, '-');

	Logger::Log(ss.str());
}