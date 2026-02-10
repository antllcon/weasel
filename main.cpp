#include "src/timer/Timer.h"
#include <iostream>
#include <windows.h>

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const Timer timer;
		std::cout << std::fixed << std::setprecision(4);
		std::cout << "Язык - Ласка!" << std::endl;
		std::cout << "Время работы: " << timer.Elapsed() << 's' << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}