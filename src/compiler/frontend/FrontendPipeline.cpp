#include "FrontendPipeline.h"
#include "src/compiler/cst_to_ast/CstToAstConverter.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/lexer/Token.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/Diagnostic.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrParser.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Frontend
{
using SourceCode = std::string;
using TokenStream = std::vector<Token>;
using CstTree = std::unique_ptr<CstNode>;
using AstTree = std::unique_ptr<AstNode>;
using SymbolTable = std::unordered_map<std::string, uint32_t>;

namespace
{

void AssertIsFileExists(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.message = "Файл не найден",
			.actual = filePath.string()});
	}
}

void AssertIsFileOpened(const std::ifstream& file, const std::filesystem::path& filePath)
{
	if (!file.is_open())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.message = "Не удалось открыть файл для чтения",
			.actual = filePath.string()});
	}
}

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
	AssertIsFileExists(filePath);
	std::ifstream file(filePath, std::ios::in | std::ios::binary);

	AssertIsFileOpened(file, filePath);
	std::ostringstream content;
	content << file.rdbuf();

	return content.str();
}

// TODO: расширить типы данных?
std::string MapTokenTypeToGrammarSymbol(const Token& token)
{
	switch (token.type)
	{
	case TokenType::Identifier:
		return "id";
	case TokenType::Integer:
	case TokenType::Float:
		return "num";
	case TokenType::String:
		return "str";
	case TokenType::Semicolon:
		return ";";
	case TokenType::EndOfFile:
		return END_SYMBOL;
	default:
		return std::string(token.value);
	}
}

CstInputToken MapTokenToCstInput(const Token& token)
{
	return CstInputToken{
		.symbol = MapTokenTypeToGrammarSymbol(token),
		.value = std::string(token.value),
		.location = SourceLocation{token.line, token.pos}};
}

std::vector<CstInputToken> MapTokensToCstInput(const TokenStream& tokens)
{
	std::vector<CstInputToken> result;
	result.reserve(tokens.size());

	for (const auto& token : tokens)
	{
		result.emplace_back(MapTokenToCstInput(token));
	}

	return result;
}

TokenStream RunLexerPhase(const SourceCode& sourceCode, DiagnosticEngine& engine)
{
	ScopedTimer t("Лексический анализ");
	return Lexer::Tokenize(sourceCode, engine);
}

CstTree RunParserPhase(
	const TokenStream& tokens,
	const LanguageContext& context,
	const std::filesystem::path& sourceFile)
{
	ScopedTimer t("Синтаксический анализ");
	LalrParser parser(*context.lalrTable);

	const auto cstTokens = MapTokensToCstInput(tokens);
	auto cstRoot = parser.ParseToTree(cstTokens);

	AssertIsCstValid(cstRoot.get(), sourceFile);
	return cstRoot;
}

AstTree RunAstConversionPhase(
	const CstTree& cstRoot,
	const std::filesystem::path& sourceFile)
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

std::optional<FrontendResult> RunFrontend(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	DiagnosticEngine& engine)
{
	auto sourceCode = ReadSourceFile(sourceFile);
	auto tokens = RunLexerPhase(sourceCode, engine);

	if (engine.HasErrors())
	{
		return std::nullopt;
	}

	auto cstRoot = RunParserPhase(tokens, context, sourceFile);
	auto astRoot = RunAstConversionPhase(cstRoot, sourceFile);
	auto slotMap = RunSemanticPhase(*astRoot);

	return FrontendResult{std::move(astRoot), std::move(slotMap)};
}

} // namespace Frontend