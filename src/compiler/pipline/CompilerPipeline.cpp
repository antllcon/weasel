#include "CompilerPipeline.h"
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/codegen/CodeGenerator.h"
#include "src/compiler/cst_to_ast/CstToAstConverter.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/lexer/Token.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/compiler/vm/machine/VirtualMachine.h"
#include "src/diagnostics/CompilationException.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrParseStepsPrinter.h"
#include "src/utils/logger/Logger.h"
#include "src/utils/logger/timer/ScopedTimer.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

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

void AssertIsCstValid(const std::unique_ptr<CstNode>& cstRoot)
{
	if (!cstRoot)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.errorCode = "SYS-004",
			.message = "Синтаксическое дерево (CST) не было построено"});
	}
}

void AssertIsAstValid(const std::unique_ptr<AstNode>& astRoot)
{
	if (!astRoot)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.errorCode = "SYS-005",
			.message = "Абстрактное синтаксическое дерево (AST) не было построено"});
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
	case TokenType::Semicolon:
		return ";";
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
//
CstInputToken MapTokenToCstInput(const Token& token)
{
	return CstInputToken{
		.symbol = MapTokenTypeToGrammarSymbol(token),
		.value = std::string(token.value),
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

void LogDiagnostics(const DiagnosticEngine& engine)
{
	for (const auto& diagnostic : engine.GetDiagnostics())
	{
		Logger::Log(DiagnosticEngine::FormatMessage(diagnostic));
	}
}
} // namespace

namespace CompilerPipeline
{
bool Compile(const std::filesystem::path& sourceFile, const LanguageContext& context)
{
	AssertIsContextValid(context);
	DiagnosticEngine engine;

	std::string sourceCode;
	std::vector<Token> tokens;

	{
		ScopedTimer t("Лексический анализ");
		sourceCode = ReadFileContent(sourceFile);
		tokens = Lexer::Tokenize(sourceCode, engine);
	}

	if (engine.HasErrors())
	{
		LogDiagnostics(engine);
		return false;
	}

	const auto grammarTokens = MapTokensToGrammar(tokens);

	LalrParser parser(*context.lalrTable);
	std::unique_ptr<CstNode> cstRoot;

	try
	{
		ScopedTimer t("Синтаксический анализ (LALR-1)");
		const auto cstTokens = MapTokensToCstInput(tokens);
		cstRoot = parser.ParseToTree(cstTokens);
	}
	catch (const std::exception& e)
	{
		LalrParseStepsPrinter::Print(parser.GetLastParseSteps(), sourceFile.string());

		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.errorCode = "SYN-001",
			.message = e.what(),
			.filePath = sourceFile.string()});

		LogDiagnostics(engine);
		return false;
	}

	AssertIsCstValid(cstRoot);
	std::unique_ptr<AstNode> astRoot;

	try
	{
		ScopedTimer t("Конвертация CST в AST");
		astRoot = CstToAstConverter::Convert(*cstRoot);
		AssertIsAstValid(astRoot);
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.errorCode = "AST-001",
			.message = e.what(),
			.filePath = sourceFile.string()});
		LogDiagnostics(engine);
		return false;
	}

	std::unordered_map<std::string, uint32_t> slotMap;

	try
	{
		ScopedTimer t("Семантический анализ");
		SemanticAnalyzer sema;
		slotMap = sema.Analyze(*astRoot);
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::Semantic,
			.errorCode = "SEM-001",
			.message = e.what(),
			.filePath = sourceFile.string()});
		LogDiagnostics(engine);
		return false;
	}

	try
	{
		Chunk finalBytecode;

		{
			ScopedTimer t("Генерация кода (Backend)");
			CodeGenerator backend(std::move(slotMap));
			finalBytecode = backend.Generate(*astRoot);
		}

		{
			ScopedTimer t("Выполнение в VM");
			VirtualMachine vm;
			vm.RegisterNativeFunction(0, [](std::span<const Value> args) -> Value {
				if (!args.empty())
				{
					std::cout << args[0].As<uint32_t>() << std::endl;
				}
				return Value(static_cast<uint32_t>(0));
			});
			vm.Interpret(finalBytecode);
		}
	}
	catch (const std::exception& e)
	{
		engine.Report(DiagnosticData{
			.phase = CompilerPhase::VirtualMachine,
			.errorCode = "VM-001",
			.message = e.what(),
			.filePath = sourceFile.string()});
		LogDiagnostics(engine);
		return false;
	}

	return true;
}
} // namespace CompilerPipeline