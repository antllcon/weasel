#include "LanguageContextBuilder.h"
#include "src/diagnostics/CompilationException.h"
#include "src/grammar/lalr/LalrTableBuilder.h"
#include "src/grammar/lalr/LalrTablePrinter.h"
#include "src/grammar/optimizer/GrammarOptimizer.h"
#include "src/grammar/parser/GrammarConsistencyChecker.h"
#include "src/grammar/parser/GrammarParser.h"
#include <filesystem>

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

std::string DetermineStartSymbol(const raw::Rules& rules)
{
	if (rules.empty())
	{
		return "S";
	}

	return rules.front().name;
}

raw::Rules ExtractAndCheckRules(const std::filesystem::path& grammarFile)
{
	raw::Rules rules = GrammarParser::ParseFile(grammarFile);
	GrammarConsistencyChecker::Check(rules);
	return rules;
}
} // namespace

namespace LanguageContextBuilder
{
LanguageContext Build(const std::filesystem::path& grammarFile, const std::shared_ptr<ILogger>& logger)
{
	AssertIsFileExisting(grammarFile);

	auto rules = ExtractAndCheckRules(grammarFile);
	const auto originalStartSymbol = DetermineStartSymbol(rules);

	std::string augmentedStartSymbol;

	rules = GrammarOptimizer::OptimizeForLalr(std::move(rules), originalStartSymbol);
	rules = GrammarOptimizer::AugmentGrammarLalr(std::move(rules), originalStartSymbol, augmentedStartSymbol);

	LalrTableBuilder tableBuilder(rules, augmentedStartSymbol);
	auto lalrTable = std::make_unique<LalrTable>(tableBuilder.Build());

	if (logger)
	{
		// LalrTablePrinter::Print(*lalrTable, logger);
	}

	return LanguageContext{
		.lalrTable = std::move(lalrTable),
		.startSymbol = std::move(augmentedStartSymbol)};
}
} // namespace LanguageContextBuilder