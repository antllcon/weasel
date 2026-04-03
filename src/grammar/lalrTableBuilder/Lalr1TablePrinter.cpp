#include "Lalr1TablePrinter.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>

namespace
{
std::set<std::string> ExtractTerminals(const LalrTable& table)
{
	std::set<std::string> terminals;
	for (const auto& row : table.actionTable)
	{
		for (const auto& symbol : row | std::views::keys)
		{
			terminals.insert(symbol);
		}
	}
	return terminals;
}

std::set<std::string> ExtractNonTerminals(const LalrTable& table)
{
	std::set<std::string> nonTerminals;
	for (const auto& row : table.gotoTable)
	{
		for (const auto& symbol : row | std::views::keys)
		{
			nonTerminals.insert(symbol);
		}
	}
	return nonTerminals;
}

std::string FormatAction(const LalrAction& action)
{
	if (action.type == LalrActionType::Shift)
	{
		return "S" + std::to_string(action.ruleIndex);
	}
	if (action.type == LalrActionType::Reduce)
	{
		return "R" + std::to_string(action.ruleIndex);
	}
	if (action.type == LalrActionType::Accept)
	{
		return "ACC";
	}
	return "-";
}

void PrintCell(const std::string& text, int width)
{
	std::cout << " " << std::left << std::setw(width) << text << " |";
}

void PrintRowSeparator(size_t totalWidth)
{
	std::cout << std::string(totalWidth, '-') << std::endl;
}
} // namespace

void Lalr1TablePrinter::Print(const LalrTable& table)
{
	std::cout << "LALR(1) Таблица разбора" << std::endl;

	const std::set<std::string> terminals = ExtractTerminals(table);
	const std::set<std::string> nonTerminals = ExtractNonTerminals(table);

	constexpr int CELL_WIDTH = 6;
	const size_t totalWidth = 2 + (1 + CELL_WIDTH + 2) * (1 + terminals.size() + nonTerminals.size());

	PrintRowSeparator(totalWidth);

	std::cout << "|";
	PrintCell("State", CELL_WIDTH);
	for (const auto& term : terminals)
	{
		PrintCell(term, CELL_WIDTH);
	}
	for (const auto& nt : nonTerminals)
	{
		PrintCell(nt, CELL_WIDTH);
	}
	std::cout << std::endl;

	PrintRowSeparator(totalWidth);

	for (size_t i = 0; i < table.actionTable.size(); ++i)
	{
		std::cout << "|";
		PrintCell(std::to_string(i), CELL_WIDTH);

		for (const auto& term : terminals)
		{
			if (table.actionTable[i].contains(term))
			{
				PrintCell(FormatAction(table.actionTable[i].at(term)), CELL_WIDTH);
			}
			else
			{
				PrintCell("", CELL_WIDTH);
			}
		}

		for (const auto& nt : nonTerminals)
		{
			if (table.gotoTable[i].contains(nt))
			{
				PrintCell(std::to_string(table.gotoTable[i].at(nt)), CELL_WIDTH);
			}
			else
			{
				PrintCell("", CELL_WIDTH);
			}
		}
		std::cout << std::endl;
	}

	PrintRowSeparator(totalWidth);
}