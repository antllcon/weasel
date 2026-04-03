#include "ParseStepsPrinter.h"
#include <iomanip>
#include <iostream>

void ParseStepsPrinter::Print(const std::vector<ParseStep>& steps, const std::string& inputLine)
{
	std::cout << std::endl;
	std::cout << "Процесс разбора строки: " << inputLine << std::endl;
	std::cout << std::string(80, '-') << std::endl;
	std::cout << std::left << std::setw(8) << "Step"
			  << std::setw(30) << "Input [parsed]no_parsed"
			  << std::setw(20) << "Stack"
			  << "Action" << std::endl;
	std::cout << std::string(80, '-') << std::endl;

	for (const auto& step : steps)
	{
		std::cout << std::left << std::setw(8) << step.stepNumber
				  << std::setw(30) << step.input
				  << std::setw(20) << step.stack
				  << "-> " << step.action << std::endl;
	}

	std::cout << std::string(80, '-') << std::endl;
	std::cout << "Разбор успешно завершен!" << std::endl;
}