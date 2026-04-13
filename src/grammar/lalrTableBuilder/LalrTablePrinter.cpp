#include "LalrTablePrinter.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

namespace
{
std::vector<std::string> ExtractOrderedSymbols(const LalrTable& table, bool wantTerminals)
{
	std::vector<std::string> result;

	auto addSymbol = [&](const std::string& sym) {
		if (sym == END_SYMBOL || sym == EMPTY_SYMBOL || sym == "Z'")
		{
			return;
		}
		if (IsTerm(sym) == wantTerminals)
		{
			if (std::ranges::find(result, sym) == result.end())
			{
				result.push_back(sym);
			}
		}
	};

	for (const auto& rule : table.augmentedRules)
	{
		addSymbol(rule.name);
		for (const auto& alt : rule.alternatives)
		{
			for (const auto& sym : alt)
			{
				addSymbol(sym);
			}
		}
	}
	return result;
}

size_t GetFlatRuleIndex(const LalrTable& table, size_t ruleIndex, size_t altIndex)
{
	size_t flatIndex = 0;
	for (size_t i = 0; i < table.augmentedRules.size(); ++i)
	{
		if (i == ruleIndex)
		{
			return flatIndex + altIndex;
		}
		flatIndex += table.augmentedRules[i].alternatives.size();
	}
	return flatIndex;
}

std::string FormatAction(const LalrAction& action, const LalrTable& table)
{
	if (action.type == LalrActionType::Shift)
	{
		return "S" + std::to_string(action.ruleIndex);
	}
	if (action.type == LalrActionType::Reduce)
	{
		size_t flatId = GetFlatRuleIndex(table, action.ruleIndex, action.altIndex);
		return "R" + std::to_string(flatId);
	}
	if (action.type == LalrActionType::Accept)
	{
		return "WIN";
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

void PrintLegend()
{
	std::cout << "\nРасшифровка действий:\n";
	std::cout << "  Sn  (Shift)  : Сдвиг - читаем токен, кладем в стек и переходим в состояние 'n'\n";
	std::cout << "  Rn  (Reduce) : Свертка по правилу номер 'n'\n";
	std::cout << "  WIN (Accept) : Строка успешно разобрана\n";
	std::cout << "  n   (Goto)   : В какое состояние переходим после свертки нетерминала\n";
}

} // namespace

void LalrTablePrinter::Print(const LalrTable& table)
{
	std::cout << "LALR(1) Таблица разбора" << std::endl;

	PrintLegend();

	std::vector<std::string> terminals = ExtractOrderedSymbols(table, true);
	const std::vector<std::string> nonTerminals = ExtractOrderedSymbols(table, false);

	bool hasEndSymbol = false;
	for (const auto& row : table.actionTable)
	{
		if (row.contains(END_SYMBOL))
		{
			hasEndSymbol = true;
			break;
		}
	}

	constexpr int CELL_WIDTH = 6;
	const size_t totalWidth = -1 + 2 + (1 + CELL_WIDTH + 2) * (1 + nonTerminals.size() + terminals.size() + (hasEndSymbol ? 1 : 0));

	PrintRowSeparator(totalWidth);

	std::cout << "|";
	PrintCell("State", CELL_WIDTH);

	for (const auto& nt : nonTerminals)
	{
		PrintCell(nt, CELL_WIDTH);
	}
	for (const auto& term : terminals)
	{
		PrintCell(term, CELL_WIDTH);
	}
	if (hasEndSymbol)
	{
		PrintCell(END_SYMBOL, CELL_WIDTH);
	}
	std::cout << std::endl;

	PrintRowSeparator(totalWidth);

	for (size_t i = 0; i < table.actionTable.size(); ++i)
	{
		std::cout << "|";
		PrintCell(std::to_string(i), CELL_WIDTH);

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

		for (const auto& term : terminals)
		{
			if (table.actionTable[i].contains(term))
			{
				PrintCell(FormatAction(table.actionTable[i].at(term), table), CELL_WIDTH);
			}
			else
			{
				PrintCell("", CELL_WIDTH);
			}
		}

		if (hasEndSymbol)
		{
			if (table.actionTable[i].contains(END_SYMBOL))
			{
				PrintCell(FormatAction(table.actionTable[i].at(END_SYMBOL), table), CELL_WIDTH);
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