#include "WkitConfig.h"

#include "src/compiler/core/LanguageTokens.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/reader/SourceLoader.h"
#include "src/diagnostics/DiagnosticEngine.h"

#include <map>
#include <stdexcept>

namespace
{
bool IsDeclarationKeyword(std::string_view value)
{
	return value == LanguageTokens::KwLet || value == LanguageTokens::KwVal || value == LanguageTokens::KwVar;
}

std::map<std::string, std::string> CollectStringConstants(const std::vector<Token>& tokens)
{
	std::map<std::string, std::string> result;

	for (size_t i = 0; i + 4 < tokens.size(); ++i)
	{
		if (tokens[i].type != TokenType::Keyword || !IsDeclarationKeyword(tokens[i].value))
		{
			continue;
		}

		const Token& keyToken = tokens[i + 2];
		const Token& assignToken = tokens[i + 3];
		const Token& valueToken = tokens[i + 4];

		const bool isPattern = keyToken.type == TokenType::Identifier
			&& assignToken.value == "="
			&& valueToken.type == TokenType::String;
		if (!isPattern)
		{
			continue;
		}

		result[std::string(keyToken.value)] = std::string(valueToken.value);
	}

	return result;
}

std::string ValueOr(const std::map<std::string, std::string>& values, const std::string& key, const std::string& fallback)
{
	const auto it = values.find(key);
	return it != values.end() ? it->second : fallback;
}
} // namespace

WkitConfig WkitConfigLoader::Load(const std::filesystem::path& configFile)
{
	if (!std::filesystem::exists(configFile))
	{
		throw std::runtime_error("Файл конфигурации не найден: " + configFile.string());
	}

	const std::string source = SourceLoader::Read(configFile);
	DiagnosticEngine engine;
	const auto tokens = Lexer::Tokenize(source, engine);
	const auto values = CollectStringConstants(tokens);

	WkitConfig config;
	config.name = ValueOr(values, "name", "");
	config.entry = ValueOr(values, "entry", "");
	config.stdlib = ValueOr(values, "stdlib", "");
	return config;
}
