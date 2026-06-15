#include "WkitCommands.h"
#include "src/utils/console/ConsoleEncoding.h"

#include <iostream>
#include <string>

namespace
{
void PrintUsage()
{
	std::cout << "wkit — инструмент управления проектами Weasel" << std::endl;
	std::cout << "Использование:" << std::endl;
	std::cout << "  wkit new <name>   создать новый проект" << std::endl;
	std::cout << "  wkit build        скомпилировать и проверить (без запуска)" << std::endl;
	std::cout << "  wkit run          скомпилировать и запустить" << std::endl;
	std::cout << "  wkit debug        запустить с подробным логом компиляции (как -l)" << std::endl;
	std::cout << "  wkit format       отформатировать исходники проекта" << std::endl;
}
} // namespace

int main(int argc, char* argv[])
{
	ConsoleEncoding console;

	try
	{
		if (argc < 2)
		{
			PrintUsage();
			return EXIT_FAILURE;
		}

		const std::string command = argv[1];

		if (command == "new")
		{
			if (argc < 3)
			{
				std::cerr << "wkit: укажите имя проекта: wkit new <name>" << std::endl;
				return EXIT_FAILURE;
			}
			return WkitCommands::New(argv[2]);
		}
		if (command == "build")
		{
			return WkitCommands::Build();
		}
		if (command == "run")
		{
			return WkitCommands::Run();
		}
		if (command == "debug")
		{
			return WkitCommands::Debug();
		}
		if (command == "format")
		{
			return WkitCommands::Format();
		}

		std::cerr << "wkit: неизвестная команда '" << command << "'" << std::endl;
		PrintUsage();
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
