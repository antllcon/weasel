#include "Ll1Parser.h"
#include "src/grammar/GrammarTypes.h"
#include <sstream>
#include <stdexcept>

namespace
{
	void AssertIsTokensNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Входная строка пуста, разбор невозможен");
		}
	}

	void AssertIsStartSymbolFound(bool isFound)
	{
		if (!isFound)
		{
			throw std::runtime_error("Стартовый символ не найден в таблице разбора");
		}
	}

	void AssertIsTokenMatched(bool isMatched, const std::string& expected, const std::string& actual, size_t pos)
	{
		if (!isMatched)
		{
			throw std::runtime_error("Синтаксическая ошибка: ожидался токен '" + expected + "', получен '" + actual + "' на позиции " + std::to_string(pos));
		}
	}

	void AssertIsSyntaxCorrect(bool isCorrect, const std::string& lookahead, size_t pos)
	{
		if (!isCorrect)
		{
			throw std::runtime_error("Синтаксическая ошибка: непредвиденный токен '" + lookahead + "' на позиции " + std::to_string(pos));
		}
	}

	std::string FormatInput(const std::vector<std::string>& tokens, size_t pos)
	{
		std::ostringstream ss;
		ss << "[";
		for (size_t i = 0; i < pos; ++i)
		{
			ss << tokens[i];
		}
		ss << "]";
		for (size_t i = pos; i < tokens.size(); ++i)
		{
			ss << tokens[i];
		}
		return ss.str();
	}

	std::string FormatStack(const std::vector<size_t>& stack)
	{
		if (stack.empty())
		{
			return "-";
		}

		std::ostringstream ss;
		for (size_t i = 0; i < stack.size(); ++i)
		{
			ss << stack[i];
			if (i + 1 < stack.size())
			{
				ss << ", ";
			}
		}
		return ss.str();
	}

	bool ContainsGuide(const std::string& guidesStr, const std::string& lookahead)
	{
		std::stringstream ss(guidesStr);
		std::string item;
		while (std::getline(ss, item, ','))
		{
			if (item == lookahead)
			{
				return true;
			}
		}
		return false;
	}

	size_t FindStartRuleIndex(const Ll1Table& table, const std::string& startSymbol)
	{
		for (const auto& row : table)
		{
			if (row.name == startSymbol)
			{
				return row.index;
			}
		}
		return SIZE_MAX;
	}
} // namespace

Ll1Parser::Ll1Parser(Ll1Table table, std::string startSymbol)
	: m_table(std::move(table))
	, m_start(std::move(startSymbol))
{
}

std::vector<ParseStep> Ll1Parser::Parse(const std::vector<std::string>& tokens) const
{
	AssertIsTokensNotEmpty(tokens.empty());

	std::vector<ParseStep> steps;
	std::vector<size_t> stack;

	size_t ip = 0;
	size_t stepCounter = 1;
	size_t curr = FindStartRuleIndex(m_table, m_start);

	AssertIsStartSymbolFound(curr != SIZE_MAX);

	bool isAccepted = false;

	while (!isAccepted)
	{
		const std::string lookahead = (ip < tokens.size()) ? tokens[ip] : END_SYMBOL;
		const auto& row = m_table[curr - 1];

		ParseStep step;
		step.stepNumber = stepCounter++;
		step.input = FormatInput(tokens, ip);
		step.stack = FormatStack(stack);

		if (row.shift)
		{
			AssertIsTokenMatched(lookahead == row.name, row.name, lookahead, ip);

			step.action = "Распознал " + row.name;
			ip++;

			if (row.transition.has_value())
			{
				curr = row.transition.value();
			}
			else
			{
				if (stack.empty())
				{
					isAccepted = true;
				}
				else
				{
					curr = stack.back();
					stack.pop_back();
				}
			}
		}
		else
		{
			if (ContainsGuide(row.guides, lookahead))
			{
				if (row.stack)
				{
					step.action = "Вызов " + row.name + " (Пушим " + std::to_string(curr + 1) + ")";
					stack.push_back(curr + 1);
					curr = row.transition.value();
				}
				else
				{
					if (row.transition.has_value())
					{
						step.action = "Переход к " + std::to_string(row.transition.value()) + " (" + row.name + ")";
						curr = row.transition.value();
					}
					else
					{
						step.action = "Епштейн вернулся";
						if (stack.empty())
						{
							isAccepted = true;
						}
						else
						{
							curr = stack.back();
							stack.pop_back();
						}
					}
				}
			}
			else
			{
				AssertIsSyntaxCorrect(!row.error, lookahead, ip);
				step.action = "Следующая альтернатива";
				curr++;
			}
		}

		steps.push_back(std::move(step));
	}

	return steps;
}