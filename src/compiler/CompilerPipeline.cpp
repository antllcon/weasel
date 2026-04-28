#include "CompilerPipeline.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/cst/CstInputToken.h"
#include "src/grammar/lalr/LalrParseStepsPrinter.h"
#include "src/lexer/Lexer.h"
#include <fstream>
#include <sstream>

namespace
{
void AssertIsFileExisting(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "SYS-001",
			.message = "Файл не найден",
			.actual = filePath.string()});
	}
}

void AssertIsFileOpen(const std::ifstream& file, const std::filesystem::path& filePath)
{
	if (!file)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "SYS-002",
			.message = "Не удалось открыть файл для чтения",
			.actual = filePath.string()});
	}
}

void AssertIsContextValid(const LanguageContext& context)
{
	if (!context.lalrTable)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.errorCode = "SYS-003",
			.message = "LALR таблица не инициализирована"});
	}
}

std::string ReadFileContent(const std::filesystem::path& filePath)
{
	AssertIsFileExisting(filePath);

	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	AssertIsFileOpen(file, filePath);

	std::ostringstream content;
	content << file.rdbuf();

	return content.str();
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
	case TokenType::EndOfFile:
		return END_SYMBOL;
	default:
		return std::string(token.value);
	}
}

std::vector<std::string> MapTokensToGrammar(const std::vector<Token>& tokens)
{
	std::vector<std::string> grammarTokens;
	grammarTokens.reserve(tokens.size());

	for (const auto& token : tokens)
	{
		grammarTokens.emplace_back(MapTokenTypeToGrammarSymbol(token));
	}

	return grammarTokens;
}

CstInputToken MapTokenToCstInput(const Token& token)
{
	return CstInputToken{
		.symbol = MapTokenTypeToGrammarSymbol(token),
		.value = token.value,
		.location = SourceLocation{token.line, token.pos}};
}

std::vector<CstInputToken> MapTokensToCstInput(const std::vector<Token>& tokens)
{
	std::vector<CstInputToken> result;
	result.reserve(tokens.size());

	for (const auto& token : tokens)
	{
		result.emplace_back(MapTokenToCstInput(token));
	}

	return result;
}
} // namespace

namespace CompilerPipeline
{
bool Compile(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	const std::shared_ptr<ILogger>& logger)
{
	AssertIsContextValid(context);

	DiagnosticEngine engine;
	const auto sourceCode = ReadFileContent(sourceFile);
	const auto tokens = Lexer::Tokenize(sourceCode, engine);

	if (engine.HasErrors())
	{
		return false;
	}

	const auto grammarTokens = MapTokensToGrammar(tokens);

	LalrParser parser(*context.lalrTable);
	const auto parseSteps = parser.Parse(grammarTokens);

	if (logger)
	{
		LalrParseStepsPrinter::Print(parseSteps, sourceFile.string(), logger);
	}

	const auto cstTokens = MapTokensToCstInput(tokens);
	const auto cstRoot = parser.ParseToTree(cstTokens);

	return true;
}
} // namespace CompilerPipeline