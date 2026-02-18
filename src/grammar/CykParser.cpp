#include "CykParser.h"
#include <stdexcept>
#include <vector>
#include <unordered_set>

namespace
{
	using CykCell = std::unordered_set<std::string>;
	using CykRow = std::vector<CykCell>;
	using CykTable = std::vector<CykRow>;

	void AssertIsGrammarNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Грамматика пуста, парсинг невозможен");
		}
	}

	void AssertIsWordNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Входная строка для разбора не должна быть пустой");
		}
	}

	CykTable InitializeTable(size_t wordLength)
	{
		return CykTable(wordLength, CykRow(wordLength + 1));
	}

	void FillLengthOne(CykTable& table, const std::string& word, const raw::Rules& rules)
	{
		const size_t length = word.size();
		for (size_t i = 0; i < length; ++i)
		{
			const std::string terminal(1, word[i]);

			for (const auto& rule : rules)
			{
				for (const auto& alternative : rule.alternatives)
				{
					if (alternative.size() == 1 && alternative.front() == terminal)
					{
						table[i][1].insert(rule.name);
					}
				}
			}
		}
	}

	void CheckAndInsertRule(
		CykCell& targetCell,
		const CykCell& leftCell,
		const CykCell& rightCell,
		const raw::Rules& rules)
	{
		for (const auto& rule : rules)
		{
			for (const auto& alternative : rule.alternatives)
			{
				if (alternative.size() == 2)
				{
					const std::string& leftSymbol = alternative[0];
					const std::string& rightSymbol = alternative[1];

					if (leftCell.contains(leftSymbol) && rightCell.contains(rightSymbol))
					{
						targetCell.insert(rule.name);
					}
				}
			}
		}
	}

	void FillLengthN(CykTable& table, size_t wordLength, const raw::Rules& rules)
	{
		for (size_t l = 2; l <= wordLength; ++l)
		{
			for (size_t i = 0; i <= wordLength - l; ++i)
			{
				for (size_t k = 1; k < l; ++k)
				{
					const CykCell& leftCell = table[i][k];
					const CykCell& rightCell = table[i + k][l - k];

					CheckAndInsertRule(table[i][l], leftCell, rightCell, rules);
				}
			}
		}
	}
}

CykParser::CykParser(raw::Rules rules, std::string startSymbol)
	: m_rules(std::move(rules))
	, m_startSymbol(std::move(startSymbol))
{
}

CykParseResult CykParser::Parse(const std::string& word) const
{
	AssertIsGrammarNotEmpty(m_rules.empty());
	AssertIsWordNotEmpty(word.empty());

	const size_t length = word.size();
	CykTable table = InitializeTable(length);

	FillLengthOne(table, word, m_rules);
	FillLengthN(table, length, m_rules);

	return {table[0][length].contains(m_startSymbol), std::move(table)};
}