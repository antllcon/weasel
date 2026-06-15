#include "CompilerPipeline.h"
#include "src/compiler/backend/BackendPipeline.h"
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/utils/logger/Logger.h"

#include <iostream>

namespace
{
void LogDiagnostics(const DiagnosticEngine& engine, LogTarget logTarget)
{
	for (const auto& diagnostic : engine.GetDiagnostics())
	{
		const auto message = DiagnosticEngine::FormatMessage(diagnostic);
		std::cerr << message << std::endl;
		if (logTarget == LogTarget::File)
		{
			Logger::Log(message);
		}
	}
}

bool ExecutePhases(const CompilerOptions& options, const LanguageContext& context, DiagnosticEngine& engine)
{
	auto frontendResult = FrontendPipeline::Run(options.sourceFile, options.stdlibDir, context, engine);
	if (!frontendResult) return false;
	return BackendPipeline::Run(*frontendResult, options);
}
} // namespace

bool CompilerPipeline::Compile(const CompilerOptions& options, const LanguageContext& context)
{
	DiagnosticEngine engine;
	auto isSuccess = false;

	try
	{
		isSuccess = ExecutePhases(options, context, engine);
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

	LogDiagnostics(engine, options.logTarget);
	return isSuccess;
}