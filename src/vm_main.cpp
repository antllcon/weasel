#include "libs/console/ConsoleEncoding.h"
#include "vm/assembler/TextAssembler.h"
#include "vm/loader/BytecodeLoader.h"
#include "vm/machine/VirtualMachine.h"
#include "vm/value/Value.h"
#include <iostream>
#include <windows.h>

namespace
{
void AssertHasCorrectArgumentCount(int argc)
{
	if (argc != 2)
	{
		throw std::runtime_error("Запускай так: wesvm [file.wes | file.wesbc]");
	}
}

void PrintResult(const VirtualMachine& vm)
{
	try
	{
		const Value topValue = vm.Peek(0);
		std::cout << "[Result] Raw: " << topValue.AsRaw()
				  << " | Double: " << topValue.AsDouble() << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "[Result] Стек пуст" << std::endl;
	}
}
} // namespace

int main(int argc, char* argv[])
{
	try
	{
		ConsoleEncoding console;

		AssertHasCorrectArgumentCount(argc);
		std::filesystem::path filePath = argv[1];

		if (filePath.extension() == ".wes")
		{
			std::filesystem::path binaryPath = filePath;
			binaryPath.replace_extension(".wesbc");

			std::cout << "[Assembler] Компиляция " << filePath.filename() << " -> " << binaryPath.filename() << std::endl;
			TextAssembler::AssembleToBinary(filePath, binaryPath);

			filePath = binaryPath;
		}

		std::cout << "[Loader] Загрузка бинарного файла " << filePath.filename() << std::endl;
		const Chunk chunk = BytecodeLoader::LoadFile(filePath);

		VirtualMachine vm;
		vm.Interpret(chunk);
		PrintResult(vm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}