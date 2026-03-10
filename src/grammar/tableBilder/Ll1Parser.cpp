#include "Ll1Parser.h"
#include "TableBuilder.h"

#include <sstream>
#include <stdexcept>

namespace
{
std::string StackToString(const std::vector<int>& stack)
{
	std::ostringstream ss;

	for (auto it = stack.rbegin(); it != stack.rend(); ++it)
	{
		ss << *it << " ";
	}

	return ss.str();
}

std::string InputToString(const std::vector<std::string>& tokens, size_t pos)
{
	std::ostringstream ss;

	for (size_t i = pos; i < tokens.size(); ++i)
	{
		ss << tokens[i] << " ";
	}

	return ss.str();
}

int FindHeaderRow(
	const std::vector<ParseTableRow>& rows,
	const std::string& nonterm,
	const std::string& lookahead)
{
	for (const auto& r : rows)
	{
		if (r.kind != ParseTableRow::Kind::NonterminalHeader)
		{
			continue;
		}

		if (r.name != nonterm)
		{
			continue;
		}

		if (r.guides.contains(lookahead))
		{
			return static_cast<int>(r.idx);
		}
	}

	return -1;
}

void AssertRowExists(const std::vector<ParseTableRow>& rows, int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= rows.size())
	{
		throw std::runtime_error("Некорректный переход по таблице LL(1)");
	}
}

void AssertGuideFound(int idx, const std::string& nonterm, const std::string& lookahead)
{
	if (idx == -1)
	{
		throw std::runtime_error(
			"Нет альтернативы для нетерминала " + nonterm + " при символе " + lookahead);
	}
}
} // namespace

Ll1Parser::Ll1Parser(std::vector<ParseTableRow> tableRows, std::string startSymbol)
	: m_rows(std::move(tableRows))
	, m_start(std::move(startSymbol))
{
}

std::vector<ParseStep> Ll1Parser::Parse(const std::vector<std::string>& tokens)
{
	if (tokens.empty())
	{
		throw std::runtime_error("Входная строка пуста");
	}

	std::vector<ParseStep> steps;
	std::vector<int> stack;

	int startRow = FindHeaderRow(m_rows, m_start, tokens.front());
	AssertGuideFound(startRow, m_start, tokens.front());

	stack.push_back(startRow);

	size_t inputPos = 0;
	size_t stepCounter = 1;

	while (!stack.empty())
	{
		int rowIdx = stack.back();
		stack.pop_back();

		AssertRowExists(m_rows, rowIdx);

		const auto& row = m_rows[rowIdx];

		std::string lookahead = tokens[inputPos];
		std::string action;

		if (row.kind == ParseTableRow::Kind::NonterminalHeader)
		{
			action = "expand " + row.name;

			if (row.groupPointer != 0)
			{
				stack.push_back(static_cast<int>(startRow));
			}
		}
		else if (row.kind == ParseTableRow::Kind::RhsTerminal)
		{
			if (row.name != lookahead)
			{
				throw std::runtime_error(
					"Ожидался терминал " + row.name + ", получен " + lookahead);
			}

			action = "match " + row.name;

			++inputPos;

			if (row.nextPointer != 0)
			{
				stack.push_back(static_cast<int>(row.nextPointer));
			}
		}
		else if (row.kind == ParseTableRow::Kind::RhsNonterminal)
		{
			action = "call " + row.name;

			int header = FindHeaderRow(m_rows, row.name, lookahead);
			AssertGuideFound(header, row.name, lookahead);

			if (row.nextPointer != 0)
			{
				stack.push_back(static_cast<int>(header));
			}

			stack.push_back(header);
		}
		else if (row.kind == ParseTableRow::Kind::RhsEmpty)
		{
			action = "epsilon";

			if (row.nextPointer != 0)
			{
				stack.push_back(static_cast<int>(row.nextPointer));
			}
		}

		ParseStep step;

		step.stepNumber = stepCounter++;
		step.stack = StackToString(stack);
		step.input = InputToString(tokens, inputPos);
		step.action = action;

		step.stepNumber = stepCounter++;

		if (inputPos >= tokens.size())
		{
			break;
		}
	}

	return steps;
}