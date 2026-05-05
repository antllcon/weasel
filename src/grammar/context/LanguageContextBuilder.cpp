#include "LanguageContextBuilder.h"
#include "src/utils/cacher/FileCache.h"
#include "src/diagnostics/CompilationException.h"
#include "src/grammar/cache/LalrTableSerializer.h"
#include "src/grammar/lalr/LalrTableBuilder.h"
#include "src/grammar/lalr/LalrTablePrinter.h"
#include "src/grammar/optimizer/GrammarOptimizer.h"
#include "src/grammar/parser/GrammarConsistencyChecker.h"
#include "src/grammar/parser/GrammarParser.h"
#include "src/utils/logger/Logger.h"
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
LanguageContext Build(const std::filesystem::path& grammarFile)
{
	AssertIsFileExisting(grammarFile);

	auto rules = ExtractAndCheckRules(grammarFile);
	const auto originalStartSymbol = DetermineStartSymbol(rules);

	std::string augmentedStartSymbol;

	rules = GrammarOptimizer::OptimizeForLalr(std::move(rules), originalStartSymbol);
	rules = GrammarOptimizer::AugmentGrammarLalr(std::move(rules), originalStartSymbol, augmentedStartSymbol);

	// TODO: улушчить (вынести в метод получения lalrTable, который в себе скрывает кеш)
	const auto cachePath = std::filesystem::path(grammarFile.string() + ".cache");

	auto buildTable = [&]() -> LalrTable
	{
		LalrTableBuilder tableBuilder(rules, augmentedStartSymbol);
		return tableBuilder.Build();
	};

	auto rebuildAndCache = [&]() -> LalrTable
	{
		auto table = buildTable();
		const auto payload = LalrTableSerializer::Serialize(table);
		FileCache::Write(grammarFile, cachePath, payload);
		return table;
	};

	LalrTable table;
	if (FileCache::IsUpToDate(grammarFile, cachePath))
	{
		try
		{
			const auto payload = FileCache::ReadPayload(cachePath);
			table = LalrTableSerializer::Deserialize(payload);

			Logger::Log("[Cache] \tLALR таблица загружена из кэша");
		}
		catch (...)
		{
			table = rebuildAndCache();
		}
	}
	else
	{
		table = rebuildAndCache();
	}

	auto lalrTable = std::make_unique<LalrTable>(std::move(table));

	// LalrTablePrinter::Print(*lalrTable);

	return LanguageContext{
		.lalrTable = std::move(lalrTable),
		.startSymbol = std::move(augmentedStartSymbol)};
}
} // namespace LanguageContextBuilder