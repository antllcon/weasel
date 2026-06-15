#include "FrontendPipeline.h"
#include "src/compiler/ast/visualizer/AstVisualizer.h"
#include "src/compiler/modules/ModuleLoader.h"
#include "src/compiler/semantic/SemanticAnalyzer.h"
#include "src/compiler/semantic/SymbolTableVisualizer.h"
#include "src/diagnostics/CompilationException.h"

namespace FrontendPipeline
{
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

void AssertIsAstValid(const AstNode* astRoot, const std::filesystem::path& sourceFile)
{
	if (!astRoot)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.message = "Абстрактное синтаксическое дерево (AST) не было построено",
			.filePath = sourceFile});
	}
}

CodegenContext RunSemanticPhase(AstNode& astRoot, DiagnosticEngine& engine)
{
	SemanticAnalyzer sema;
	auto codegenCtx = sema.Analyze(astRoot, engine);
	SymbolTableVisualizer::Visualize(codegenCtx.symbols);
	return codegenCtx;
}
} // namespace

std::optional<FrontendResult> Run(
	const std::filesystem::path& sourceFile,
	const std::filesystem::path& stdlibDir,
	const LanguageContext& context,
	DiagnosticEngine& engine)
{
	AssertIsContextValid(context);

	auto astRoot = ModuleLoader::Load(sourceFile, stdlibDir, context, engine);
	if (engine.HasErrors()) return std::nullopt;

	AssertIsAstValid(astRoot.get(), sourceFile);
	AstVisualizer::Visualize(*astRoot);

	auto semaResult = RunSemanticPhase(*astRoot, engine);
	if (engine.HasErrors()) return std::nullopt;

	return FrontendResult{
		std::move(astRoot),
		std::move(semaResult)};
}
} // namespace FrontendPipeline
