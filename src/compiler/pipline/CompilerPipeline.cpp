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

void RunVirtualBackend(Frontend::FrontendResult& result)
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

void RunNativeBackend(Frontend::FrontendResult& result, const std::filesystem::path& sourceFile)
{
	const auto outputFile = std::filesystem::path(sourceFile).replace_extension(".asm");

	ScopedTimer t("Генерация NASM x86-64");
	NasmCodeGenerator backend;

	const std::string asmText = backend.Generate(*result.ast);

	std::ofstream out(outputFile);
	AssertIsFileOpenedForWrite(out, outputFile);

	out << asmText;
	Logger::Log("NASM-код записан в: " + outputFile.string());
}

} // namespace

namespace CompilerPipeline
{

bool Compile(const CompilerOptions& options, const LanguageContext& context)
{
	DiagnosticEngine engine;

	try
	{
		AssertIsContextValid(context);

		auto result = Frontend::Run(options.sourceFile, context, engine);

		if (!result)
		{
			LogDiagnostics(engine);
			return false;
		}

		if (options.emitNasm)
		{
			RunNativeBackend(*result, options.sourceFile);
		}
		else
		{
			RunVirtualBackend(*result);
		}

		return true;
	}
	catch (const CompilationException& e)
	{
		engine.Report(e.GetData());
		LogDiagnostics(engine);
		return false;
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Fatal,
			.message = e.what()});
		LogDiagnostics(engine);
		return false;
	}
}

} // namespace CompilerPipeline