#include "CompilerPipeline.h"
#include "src/diagnostics/CompilationException.h"
#include "src/grammar/lalrTableBuilder/LalrParseStepsPrinter.h"
#include "src/grammar/lalrTableBuilder/LalrTableBuilder.h"
#include "src/grammar/lalrTableBuilder/LalrTablePrinter.h"
#include "src/grammar/parser/GrammarConsistencyChecker.h"
#include "src/grammar/parser/GrammarParser.h"
#include "src/grammar/printGrammar/PrintGrammar.h"
#include "src/lexer/Lexer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
void AssertFileExists(const std::filesystem::path& filePath)
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

std::string ReadFileContent(const std::filesystem::path& filePath)
{
	AssertFileExists(filePath);

	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	// Это должен быть одтельный Assert!
	if (!file)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "SYS-002",
			.message = "Не удалось открыть файл для чтения",
			.actual = filePath.string()});
	}

	std::ostringstream content;
	content << file.rdbuf();
	return content.str();
}

raw::Rules ExtractAndCheckRules(const std::filesystem::path& grammarFile)
{
	raw::Rules rules = GrammarParser::ParseFile(grammarFile);
	GrammarConsistencyChecker::Check(rules);
	return rules;
}

std::string_view DetermineStartSymbol(const raw::Rules& rules)
{
	if (rules.empty())
	{
		return "S";
	}
	return rules.front().name;
}

void PrintGrammarState(const raw::Rules& rules, const std::shared_ptr<ILogger>& logger, const std::string& title)
{
	PrintGrammar::PrintRules(rules, logger, title);
}
} // namespace

void CompilerPipeline::InitGrammar(const std::filesystem::path& grammarFile, const std::shared_ptr<ILogger>& logger)
{
	AssertFileExists(grammarFile);

	auto rules = ExtractAndCheckRules(grammarFile);
	const auto symbol = std::string(DetermineStartSymbol(rules));

	if (logger)
	{
		//	PrintGrammarState(rules, logger, "Исходная грамматика:");
	}

	rules = GrammarOptimizer::OptimizeForLalr(std::move(rules), symbol);
	rules = GrammarOptimizer::AugmentGrammarLalr(std::move(rules), symbol, m_startSymbol);

	if (logger)
	{
		// PrintGrammarState(rules, logger, "Подготовленная грамматика для LALR(1):");
	}

	LalrTableBuilder tableBuilder(rules, m_startSymbol);
	m_lalrTable = std::make_unique<LalrTable>(tableBuilder.Build());

	if (logger)
	{
		LalrTablePrinter::Print(*m_lalrTable, logger);
	}
}

bool CompilerPipeline::Compile(const std::filesystem::path& sourceFile, const std::shared_ptr<ILogger>& logger)
{
	// TODO: вынести отдельно
	if (!m_lalrTable)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.errorCode = "SYS-003",
			.message = "LALR таблица не инициализирована"});
	}

	m_engine.Clear();

	const auto sourceCode = ReadFileContent(sourceFile);
	const auto tokens = Lexer::Tokenize(sourceCode, m_engine);

	if (m_engine.HasErrors())
	{
		return false;
	}

	std::vector<std::string> grammarTokens;
	grammarTokens.reserve(tokens.size());

	// TODO: подумать где будут объявлены ключевые слова
	for (const auto& token : tokens)
	{
		switch (token.type)
		{
		case TokenType::Identifier:
			grammarTokens.emplace_back("id");
			break;
		case TokenType::Integer:
		case TokenType::Float:
			grammarTokens.emplace_back("num");
			break;
		case TokenType::String:
			grammarTokens.emplace_back("str");
			break;
		case TokenType::EndOfFile:
			grammarTokens.emplace_back(END_SYMBOL);
			break;
		default:
			grammarTokens.emplace_back(token.value);
			break;
		}
	}

	LalrParser parser(*m_lalrTable);
	const auto parseSteps = parser.Parse(grammarTokens);

	if (logger)
	{
		LalrParseStepsPrinter::Print(parseSteps, sourceFile.string(), logger);
	}

	return true;
}