#include "grammar/GrammarTypes.h"
#include "vm/machine/VirtualMachine.h"
#include "vm/parser/BytecodeParser.h"
#include "vm/value/Value.h"
#include <iostream>
#include <variant>
#include <windows.h>

namespace
{
void AssertHasCorrectArgumentCount(int argc)
{
	if (argc != 2)
	{
		throw std::runtime_error("Использование: start [name.wesbc]");
	}
}

void PrintResult(const Value& result)
{
	std::visit([](auto&& val) {
		std::cout << "[Result] " << val << std::endl;
	},
		result);
}
} // namespace

int main(int argc, char* argv[])
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		AssertHasCorrectArgumentCount(argc);

		const std::filesystem::path filePath = argv[1];
		const Chunk chunk = BytecodeParser::ParseFile(filePath);

		VirtualMachine vm;
		vm.Interpret(chunk);
		PrintResult(vm.GetLastResult());
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}