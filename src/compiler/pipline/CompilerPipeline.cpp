#include "CompilerPipeline.h"
#include "src/compiler/backend/BackendPipeline.h"
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/utils/logger/Logger.h"

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

		BackendPipeline::Run(*result, options);
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
