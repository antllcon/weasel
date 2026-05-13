#include "FrontendPipeline.h"
#include "src/compiler/cst_to_ast/CstToAstConverter.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/reader/SourceLoader.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/diagnostics/CompilationException.h"
#include "src/grammar/lalr/LalrParser.h"
#include "src/utils/logger/timer/ScopedTimer.h"

namespace FrontendPipline
{
using SourceCode = std::string;
using TokenStream = std::vector<Token>;
using CstTree = std::unique_ptr<CstNode>;
using AstTree = std::unique_ptr<AstNode>;
using SymbolTable = std::unordered_map<std::string, uint32_t>;

namespace
{
void AssertIsCstValid(const CstNode* cstRoot, const std::filesystem::path& sourceFile)
{
	if (!cstRoot)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.message = "Синтаксическое дерево (CST) не было построено",
			.filePath = sourceFile});
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

SourceCode ReadSourceFile(const std::filesystem::path& filePath)
{
	ScopedTimer t("Чтение исходного файла");
	return SourceLoader::Read(filePath);
}

TokenStream RunLexerPhase(const SourceCode& sourceCode, DiagnosticEngine& engine)
{
	ScopedTimer t("Лексический анализ");
	return Lexer::Tokenize(sourceCode, engine);
}

CstTree RunParserPhase(const TokenStream& tokens, const LanguageContext& context, const std::filesystem::path& sourceFile)
{
	ScopedTimer t("Синтаксический анализ");
	auto cstRoot = LalrParser::ParseTokenStream(*context.lalrTable, tokens, true);
	AssertIsCstValid(cstRoot.get(), sourceFile);
	return cstRoot;
}

AstTree RunAstConversionPhase(const CstTree& cstRoot, const std::filesystem::path& sourceFile)
{
	ScopedTimer t("Конвертация CST в AST");
	auto astRoot = CstToAstConverter::Convert(*cstRoot);

	AssertIsAstValid(astRoot.get(), sourceFile);
	return astRoot;
}

SymbolTable RunSemanticPhase(AstNode& astRoot)
{
	ScopedTimer t("Семантический анализ");
	SemanticAnalyzer sema;
	return sema.Analyze(astRoot);
}
} // namespace

std::optional<FrontendResult> Run(const std::filesystem::path& sourceFile, const LanguageContext& context, DiagnosticEngine& engine)
{
	auto sourceCode = ReadSourceFile(sourceFile);

	auto tokens = RunLexerPhase(sourceCode, engine);
	if (engine.HasErrors()) return std::nullopt;

	auto cstRoot = RunParserPhase(tokens, context, sourceFile);
	auto astRoot = RunAstConversionPhase(cstRoot, sourceFile);
	auto slotMap = RunSemanticPhase(*astRoot);

	return FrontendResult{std::move(astRoot), std::move(slotMap)};
}
} // namespace FrontendPipline