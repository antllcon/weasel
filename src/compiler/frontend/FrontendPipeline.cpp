#include "FrontendPipeline.h"
#include "src/compiler/cst_to_ast/CstToAstConverter.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/lexer/Token.h"
#include "src/compiler/reader/SourceLoader.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/Diagnostic.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrParser.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <optional>
#include <unordered_map>
#include <vector>

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

bool IsNoiseNewLine(const TokenStream& tokens, size_t i, TokenType prevType)
{
	if (prevType == TokenType::BraceLeft || prevType == TokenType::BraceRight)
	{
		return true;
	}

	for (size_t j = i + 1; j < tokens.size(); ++j)
	{
		if (tokens[j].type == TokenType::NewLine)
		{
			continue;
		}
		if (tokens[j].type == TokenType::BraceLeft)
		{
			return true;
		}
		if (tokens[j].type == TokenType::Keyword && tokens[j].value == "else")
		{
			return true;
		}
		break;
	}

	return false;
}

TokenStream FilterNewLines(const TokenStream& tokens)
{
	TokenStream result;
	result.reserve(tokens.size());

	TokenType prevType = TokenType::Error;

	for (size_t i = 0; i < tokens.size(); ++i)
	{
		if (tokens[i].type == TokenType::NewLine)
		{
			if (!IsNoiseNewLine(tokens, i, prevType))
			{
				result.push_back(tokens[i]);
			}
		}
		else
		{
			result.push_back(tokens[i]);
			prevType = tokens[i].type;
		}
	}

	return result;
}

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
	case TokenType::NewLine:
		return "nl";
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

CstTree RunParserPhase(const TokenStream& tokens, const LanguageContext& context, const std::filesystem::path& sourceFile)
{
	ScopedTimer t("Синтаксический анализ");
	LalrParser parser(*context.lalrTable);

	const auto filtered = FilterNewLines(tokens);
	const auto cstTokens = MapTokensToCstInput(filtered);
	auto cstRoot = parser.ParseToTree(cstTokens);

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