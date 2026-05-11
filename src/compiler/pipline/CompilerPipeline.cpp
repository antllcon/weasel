#include "CompilerPipeline.h"
#include "src/compiler/backend/nasm/NasmCodeGenerator.h"
#include "src/compiler/codegen/CodeGenerator.h"
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/compiler/vm/machine/VirtualMachine.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/utils/logger/Logger.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{

void AssertIsContextValid(const LanguageContext& context)
{
	if (!context.lalrTable)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.message = "LALR таблица не инициализирована"});
	}
}

void LogDiagnostics(const DiagnosticEngine& engine)
{
	for (const auto& diagnostic : engine.GetDiagnostics())
	{
		Logger::Log(DiagnosticEngine::FormatMessage(diagnostic));
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

bool RunVirtualBackend(Frontend::FrontendResult& result, DiagnosticEngine& engine)
{
	try
	{
		Chunk finalBytecode;

		{
			ScopedTimer t("Генерация кода (Backend)");
			CodeGenerator backend(std::move(result.slotMap));
			finalBytecode = backend.Generate(*result.ast);
		}

		{
			ScopedTimer t("Выполнение в VM");
			VirtualMachine vm;
			RegisterStdlibFunctions(vm);
			vm.Interpret(finalBytecode);
		}
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.message = e.what()});
		return false;
	}

	return true;
}

bool RunNativeBackend(
	Frontend::FrontendResult& result,
	const std::filesystem::path& sourceFile,
	DiagnosticEngine& engine)
{
	const auto outputFile = std::filesystem::path(sourceFile).replace_extension(".asm");

	try
	{
		ScopedTimer t("Генерация NASM x86-64");
		NasmCodeGenerator backend;

		const std::string asmText = backend.Generate(*result.ast);

		std::ofstream out(outputFile);
		if (!out)
		{
			throw std::runtime_error("Не удалось открыть файл для записи: " + outputFile.string());
		}
		out << asmText;
		Logger::Log("NASM-код записан в: " + outputFile.string());
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Backend,
			.message = e.what(),
			.filePath = sourceFile.string()});
		return false;
	}

	return true;
}

} // namespace

namespace CompilerPipeline
{

bool Compile(const CompilerOptions& options, const LanguageContext& context)
{
	AssertIsContextValid(context);
	DiagnosticEngine engine;

	auto result = Frontend::RunFrontend(options.sourceFile, context, engine);

	if (!result)
	{
		LogDiagnostics(engine);
		return false;
	}

	const bool success = options.emitNasm
		? RunNativeBackend(*result, options.sourceFile, engine)
		: RunVirtualBackend(*result, engine);

	if (!success)
	{
		LogDiagnostics(engine);
	}

	return success;
}

} // namespace CompilerPipeline