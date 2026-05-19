#include "BackendPipeline.h"
#include "src/compiler/backend/nasm/NasmCodeGenerator.h"
#include "src/compiler/codegen/CodeGenerator.h"
#include "src/compiler/stdlib/NativeRegistry.h"
#include "src/compiler/vm/machine/VirtualMachine.h"
#include "src/diagnostics/CompilationException.h"
#include "src/utils/logger/Logger.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <fstream>

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
	for (const auto& native : NativeRegistry::GetAll())
	{
		vm.RegisterNativeFunction(native.id, native.callback);
	}
}

Chunk GenerateBytecode(FrontendPipeline::FrontendResult& result)
{
	ScopedTimer t("Генерация кода (Backend)");
	CodeGenerator backend(
		std::move(result.symbols),
		std::move(result.repIterators),
		std::move(result.functions));
	return backend.Generate(*result.ast);
}

void RunVirtualMachine(const Chunk& bytecode)
{
	ScopedTimer t("Выполнение в VM");
	VirtualMachine vm;
	RegisterStdlibFunctions(vm);
	vm.Interpret(bytecode);
}

void RunNasmBackend(FrontendPipeline::FrontendResult& result, const CompilerOptions& options)
{
	ScopedTimer t("Генерация NASM x86-64 под Windows");
	auto outputFile = std::filesystem::path(options.sourceFile).replace_extension(".asm");
	NasmCodeGenerator backend(
		std::move(result.symbols),
		std::move(result.varDeclSlots),
		std::move(result.repIterators),
		std::move(result.functions));

	auto asmText = backend.Generate(*result.ast);

	std::ofstream out(outputFile);
	AssertIsFileOpenedForWrite(out, outputFile);

	out << asmText;
	Logger::Log("[Compiler]\tNASM-код записан в: " + outputFile.string());
}

void RunVmBackend(FrontendPipeline::FrontendResult& result)
{
	auto bytecode = GenerateBytecode(result);
	RunVirtualMachine(bytecode);
}
} // namespace

namespace BackendPipeline
{
bool Run(FrontendPipeline::FrontendResult& result, const CompilerOptions& options)
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
