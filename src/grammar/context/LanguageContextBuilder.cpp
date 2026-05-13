#include "LanguageContextBuilder.h"
#include "src/diagnostics/CompilationException.h"
#include "src/grammar/cache/LalrTableSerializer.h"
#include "src/grammar/lalr/LalrTableBuilder.h"
#include "src/grammar/optimizer/GrammarOptimizer.h"
#include "src/grammar/parser/GrammarParser.h"
#include "src/utils/cacher/FileCache.h"
#include "src/utils/logger/Logger.h"
#include <filesystem>
#include <optional>

namespace
{
void AssertIsFileExisting(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Fatal,
			.message = "Файл не найден",
			.expected = "[<grammar-name>.wesg]",
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

struct PreparedGrammar
{
	raw::Rules rules;
	std::string startSymbol;
};

PreparedGrammar PrepareGrammarForLalr(raw::Rules rules)
{
	const auto originalStart = DetermineStartSymbol(rules);
	rules = GrammarOptimizer::OptimizeForLalr(std::move(rules), originalStart);

	std::string augmentedStart;
	rules = GrammarOptimizer::AugmentGrammarLalr(std::move(rules), originalStart, augmentedStart);

	return {std::move(rules), std::move(augmentedStart)};
}

std::optional<LalrTable> TryLoadFromCache(const std::filesystem::path& cachePath)
{
	try
	{
		const auto payload = FileCache::ReadPayload(cachePath);
		Logger::Log("[Cache] \tLALR таблица загружена из кэша");
		return LalrTableSerializer::Deserialize(payload);
	}
	catch (...)
	{
		return std::nullopt;
	}
}

std::unique_ptr<LalrTable> BuildAndCacheLalrTable(
	const std::filesystem::path& grammarFile,
	const std::filesystem::path& cachePath,
	const raw::Rules& rules,
	const std::string& startSymbol)
{
	LalrTableBuilder builder(rules, startSymbol);
	auto table = builder.Build();

	const auto payload = LalrTableSerializer::Serialize(table);
	FileCache::Write(grammarFile, cachePath, payload);

	return std::make_unique<LalrTable>(std::move(table));
}

std::unique_ptr<LalrTable> GetLalrTable(
	const std::filesystem::path& grammarFile,
	const raw::Rules& rules,
	const std::string& startSymbol)
{
	const auto cachePath = std::filesystem::path(grammarFile.string() + ".ch");

	if (FileCache::IsUpToDate(grammarFile, cachePath))
	{
		auto table = TryLoadFromCache(cachePath);
		if (table)
		{
			return std::make_unique<LalrTable>(std::move(*table));
		}
	}

	return BuildAndCacheLalrTable(grammarFile, cachePath, rules, startSymbol);
}
} // namespace

namespace LanguageContextBuilder
{
LanguageContext Build(const std::filesystem::path& grammarFile)
{
	AssertIsFileExisting(grammarFile);

	auto rules = GrammarParser::ParseFile(grammarFile);
	auto [preparedRules, startSymbol] = PrepareGrammarForLalr(std::move(rules));
	auto lalrTable = GetLalrTable(grammarFile, preparedRules, startSymbol);

	return LanguageContext{
		.lalrTable = std::move(lalrTable),
		.startSymbol = std::move(startSymbol)};
}
} // namespace LanguageContextBuilder