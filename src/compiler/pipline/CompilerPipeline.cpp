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

void Run(FrontendPipline::FrontendResult& result, const CompilerOptions& options)
{
	if (options.emitNasm)
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
	else
	{
		auto finalBytecode = GenerateBytecode(result);
		RunVirtualMachine(finalBytecode);
	}
}

} // namespace

bool CompilerPipeline::Compile(const CompilerOptions& options, const LanguageContext& context)
{
	DiagnosticEngine engine;

	try
	{
		AssertIsContextValid(context);
		auto result = FrontendPipline::Run(options.sourceFile, context, engine);

		if (!result)
		{
			LogDiagnostics(engine);
			return false;
		}

		Run(*result, options);
		return true;
	}
	catch (const CompilationException& e)
	{
		engine.Report(e.GetData());
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Fatal,
			.message = e.what()});
	}

	LogDiagnostics(engine);
	return false;
}