#include "BackendPipeline.h"
#include "src/compiler/backend/nasm/NasmCodeGenerator.h"
#include "src/compiler/codegen/CodeGenerator.h"
#include "src/compiler/vm/machine/VirtualMachine.h"
#include "src/diagnostics/CompilationException.h"
#include "src/utils/logger/Logger.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <fstream>
#include <iostream>

namespace
{
void AssertIsFileOpenedForWrite(const std::ofstream& file, const std::filesystem::path& filePath)
{
	if (!file.is_open())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = "Не удалось открыть файл для записи",
			.actual = filePath.string()});
	}
}

void RegisterStdlibFunctions(VirtualMachine& vm)
{
	vm.RegisterNativeFunction(0, [](std::span<const Value> args) -> Value {
		if (!args.empty())
		{
			std::cout << args[0].As<uint32_t>() << std::endl;
		}
		return Value(static_cast<uint32_t>(0));
	});
}

Chunk GenerateBytecode(FrontendPipline::FrontendResult& result)
{
	ScopedTimer t("Генерация кода (Backend)");
	CodeGenerator backend(std::move(result.slotMap));
	return backend.Generate(*result.ast);
}

void RunVirtualMachine(const Chunk& bytecode)
{
	ScopedTimer t("Выполнение в VM");
	VirtualMachine vm;
	RegisterStdlibFunctions(vm);
	vm.Interpret(bytecode);
}

void RunNasmBackend(FrontendPipline::FrontendResult& result, const CompilerOptions& options)
{
	ScopedTimer t("Генерация NASM x86-64");
	auto outputFile = std::filesystem::path(options.sourceFile).replace_extension(".asm");
	NasmCodeGenerator backend;

	auto asmText = backend.Generate(*result.ast);

	std::ofstream out(outputFile);
	AssertIsFileOpenedForWrite(out, outputFile);

	out << asmText;
	Logger::Log("[Compiler]\tNASM-код записан в: " + outputFile.string());
}

void RunVmBackend(FrontendPipline::FrontendResult& result)
{
	auto bytecode = GenerateBytecode(result);
	RunVirtualMachine(bytecode);
}
} // namespace

namespace BackendPipeline
{
bool Run(FrontendPipline::FrontendResult& result, const CompilerOptions& options)
{
	if (options.emitNasm)
	{
		RunNasmBackend(result, options);
	}
	else
	{
		RunVmBackend(result);
	}

	return true;
}
} // namespace BackendPipeline
