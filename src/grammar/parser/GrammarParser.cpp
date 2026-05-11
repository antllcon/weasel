#include "GrammarParser.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
void AssertIsFileOpened(bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть файл с грамматикой");
	}
}

void AssertIsRuleValid(bool isValid)
{
	if (!isValid)
	{
		throw std::runtime_error("Синтаксическая ошибка: отсутствует разделитель -> в правиле");
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
	AssertIsRuleValid(arrowPos != std::string::npos);

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
	AssertIsFileOpened(file.is_open());

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

	return rules;
}