#include "logger/logger/ConsoleLogger.h"
#include "src/console/ConsoleEncoding.h"
#include "timer/ScopedTimer.h"
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

		std::cout << "[Result] Raw:    " << topValue.AsRaw() << "\n";
		std::cout << "         Double: " << topValue.As<double>() << "\n";
		std::cout << "         Single: " << topValue.As<float>() << "\n";
		std::cout << "         I64:    " << topValue.As<int64_t>() << "\n";
		std::cout << "         I32:    " << topValue.As<int32_t>() << "\n";
		std::cout << "         U64:    " << topValue.As<uint64_t>() << "\n";
		std::cout << "         U32:    " << topValue.As<uint32_t>() << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "[Result] Стек пуст" << std::endl;
	}
}
} // namespace

int main(int argc, char* argv[])
{
	auto logger = std::make_shared<ConsoleLogger>(true);

	try
	{
		SetConsoleOutputCP(CP_UTF8);
		ConsoleEncoding console;
		ScopedTimer timer("MyPhase", std::cout);

		AssertHasCorrectArgumentCount(argc);
		std::filesystem::path filePath = argv[1];

		if (filePath.extension() == ".wes")
		{
			std::filesystem::path binaryPath = filePath;
			binaryPath.replace_extension(".wesbc");

			std::cout << "[Asmblr] Компиляция " << filePath.filename() << " -> " << binaryPath.filename() << std::endl;
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