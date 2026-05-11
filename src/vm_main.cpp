#include "compiler/vm/assembler/TextAssembler.h"
#include "compiler/vm/chunk/Chunk.h"
#include "compiler/vm/exception/VmException.h"
#include "compiler/vm/loader/BytecodeLoader.h"
#include "src/compiler/vm/exception/BackendException.h"
#include "src/compiler/vm/machine/VirtualMachine.h"
#include "src/compiler/vm/value/Value.h"
#include "src/diagnostics/CompilationException.h"
#include "src/utils/console/ConsoleEncoding.h"
#include "src/utils/logger/console/ConsoleLogger.h"
#include "src/utils/logger/timer/ScopedTimer.h"
#include <iostream>

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

		std::cout << "[Result]\tRaw:\t" << topValue.AsRaw() << std::endl;
		std::cout << "        \tDouble:\t" << topValue.As<double>() << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "[Result] Стек пуст" << std::endl;
	}
}

void RunPipeline(std::filesystem::path filePath)
{
	try
	{
		if (filePath.extension() == ".wes")
		{
			std::filesystem::path binaryPath = filePath;
			binaryPath.replace_extension(".wesbc");

			std::cout << "[Assembler]\tКомпиляция " << filePath.filename() << " -> " << binaryPath.filename() << std::endl;
			TextAssembler::AssembleToBinary(filePath, binaryPath);

			filePath = binaryPath;
		}

		std::cout << "[Loader]\tЗагрузка бинарного файла " << filePath.filename() << std::endl;
		const Chunk chunk = BytecodeLoader::LoadFile(filePath);

		VirtualMachine vm;
		vm.Interpret(chunk);
		PrintResult(vm);
	}
	catch (const VmException& e)
	{
		DiagnosticData data;
		data.phase = CompilerPhase::VirtualMachine;
		data.errorCode = e.GetErrorCode();
		data.message = e.GetErrorMessage();
		data.line = e.GetLine();
		data.filePath = filePath.string();
		throw CompilationException(data);
	}
	catch (const BackendException& e)
	{
		DiagnosticData data;
		data.phase = e.GetPhase();
		data.errorCode = e.GetErrorCode();
		data.message = e.GetErrorMessage();
		data.filePath = filePath.string();
		throw CompilationException(data);
	}
}
} // namespace

int main(int argc, char* argv[])
{
	ConsoleEncoding console;
	auto logger = std::make_shared<ConsoleLogger>(true, false);

	try
	{
		ScopedTimer timer("Исполнение (VM)");
		AssertHasCorrectArgumentCount(argc);

		RunPipeline(argv[1]);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}