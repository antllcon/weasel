#include "GrammarParser.h"
#include "src/diagnostics/CompilationException.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace
{
void AssertIsFileOpened(bool isOpen, const std::filesystem::path& path)
{
	if (!isOpen)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.message = "Не удалось открыть файл с грамматикой",
			.actual = path.string()});
	}
}

void AssertIsRuleValid(bool isValid, const std::string& line)
{
	if (!isValid)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Parser,
			.message = "Синтаксическая ошибка: отсутствует разделитель ->",
			.expected = "NonTerminal -> ...",
			.actual = line});
	}
}

void AssertIsAllNonTerminalsDefined(const raw::Rules& rules)
{
	std::unordered_set<std::string> defined;
	for (const auto& rule : rules)
	{
		defined.insert(rule.name);
	}

	for (const auto& [name, alternatives] : rules)
	{
		for (const auto& alt : alternatives)
		{
			for (const auto& symbol : alt)
			{
				if (!IsTerm(symbol) && !defined.contains(symbol))
				{
					throw CompilationException(DiagnosticData{
						.phase = CompilerPhase::Parser,
						.message = "Нетерминал '" + symbol + "' используется, но не определён",
						.actual = symbol});
				}
			}
		}
	}
}

std::string TrimWhitespace(const std::string& str)
{
	const size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return "";
	}
	const size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::vector<std::string> TokenizeAlternative(const std::string& altStr)
{
	std::vector<std::string> symbols;
	size_t i = 0;
	const size_t length = altStr.size();

	while (i < length)
	{
		if (std::isspace(static_cast<unsigned char>(altStr[i])))
		{
			i++;
			continue;
		}

		if (std::isalpha(static_cast<unsigned char>(altStr[i])))
		{
			const size_t start = i;
			while (i < length && (std::isalnum(static_cast<unsigned char>(altStr[i])) || altStr[i] == '_'))
			{
				i++;
			}
			while (i < length && altStr[i] == '\'')
			{
				i++;
			}
			symbols.push_back(altStr.substr(start, i - start));
		}
		else if (std::ispunct(static_cast<unsigned char>(altStr[i])))
		{
			if (altStr[i] == '(' || altStr[i] == ')' || altStr[i] == '[' || altStr[i] == ']')
			{
				symbols.push_back(std::string(1, altStr[i]));
				i++;
			}
			else
			{
				const size_t start = i;
				while (i < length && std::ispunct(static_cast<unsigned char>(altStr[i])) && altStr[i] != '(' && altStr[i] != ')' && altStr[i] != '[' && altStr[i] != ']')
				{
					i++;
				}
				symbols.push_back(altStr.substr(start, i - start));
			}
		}
		else
		{
			symbols.push_back(std::string(1, altStr[i]));
			i++;
		}
	}

	if (symbols.empty())
	{
		symbols.push_back(EMPTY_SYMBOL);
	}

	return symbols;
}

raw::Alternatives ParseAlternatives(const std::string& rhsStr)
{
	raw::Alternatives alternatives;
	std::istringstream stream(rhsStr);
	std::string altToken;

	while (std::getline(stream, altToken, '|'))
	{
		raw::Alternative alt = TokenizeAlternative(TrimWhitespace(altToken));
		alternatives.push_back(std::move(alt));
	}

	return alternatives;
}

void ParseLine(const std::string& line, raw::Rules& rules)
{
	const std::string trimmedLine = TrimWhitespace(line);
	if (trimmedLine.empty())
	{
		return;
	}

	const size_t arrowPos = trimmedLine.find("->");
	AssertIsRuleValid(arrowPos != std::string::npos, trimmedLine);

	const std::string lhs = TrimWhitespace(trimmedLine.substr(0, arrowPos));
	const std::string rhs = TrimWhitespace(trimmedLine.substr(arrowPos + 2));

	raw::Alternatives newAlts = ParseAlternatives(rhs);

	auto it = std::ranges::find_if(rules, [&lhs](const raw::Rule& r) {
		return r.name == lhs;
	});

	if (it != rules.end())
	{
		it->alternatives.insert(
			it->alternatives.end(),
			std::make_move_iterator(newAlts.begin()),
			std::make_move_iterator(newAlts.end()));
	}
	else
	{
		raw::Rule rule;
		rule.name = lhs;
		rule.alternatives = std::move(newAlts);
		rules.push_back(std::move(rule));
	}
}
} // namespace

raw::Rules GrammarParser::ParseFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	AssertIsFileOpened(file.is_open(), path);

	std::stringstream buffer;
	buffer << file.rdbuf();

	return ParseString(buffer.str());
}

raw::Rules GrammarParser::ParseString(const std::string& content)
{
	raw::Rules rules;
	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line))
	{
		ParseLine(line, rules);
	}

	AssertIsAllNonTerminalsDefined(rules);
	return rules;
}