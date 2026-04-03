#include "LalrParseStepsPrinter.h"
#include <iomanip>
#include <iostream>

void LalrParseStepsPrinter::Print(const std::vector<LalrParseStep>& steps, const std::string& inputLine)
{
	std::cout << std::endl;
	std::cout << "Процесс восходящего разбора (LALR-1) строки: " << inputLine << std::endl;
	std::cout << std::string(100, '-') << std::endl;
	std::cout << std::left << std::setw(8) << "Step"
			  << std::setw(20) << "State Stack"
			  << std::setw(20) << "Symbol Stack"
			  << std::setw(25) << "Input"
			  << "Action" << std::endl;
	std::cout << std::string(100, '-') << std::endl;

	for (const auto& step : steps)
	{
		std::cout << std::left << std::setw(8) << step.stepNumber
				  << std::setw(20) << step.stateStack
				  << std::setw(20) << step.symbolStack
				  << std::setw(25) << step.input
				  << "-> " << step.action << std::endl;
	}

	std::cout << std::string(100, '-') << std::endl;
	std::cout << "Разбор успешно завершен!" << std::endl;
}