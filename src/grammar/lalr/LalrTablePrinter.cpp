#include "LalrTablePrinter.h"
#include "src/utils/logger/Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
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
		const size_t flatId = GetFlatRuleIndex(table, action.ruleIndex, action.altIndex);
		return "R" + std::to_string(flatId);
	}
	if (action.type == LalrActionType::Accept)
	{
		return "WIN";
	}
	return "-";
}

void PrintCell(std::ostringstream& ss, const std::string& text, size_t width)
{
	ss << " " << std::left << std::setw(static_cast<int>(width)) << text << " |";
}

void PrintRowSeparator(std::ostringstream& ss, size_t totalWidth)
{
	ss << std::string(totalWidth, '-') << "\n";
}

void PrintLegend(std::ostringstream& ss)
{
	ss << "\nРасшифровка действий:\n";
	ss << "Sn  (Shift)  : Сдвиг - читаем токен, кладем в стек и переходим в состояние 'n'\n";
	ss << "Rn  (Reduce) : Свертка по правилу номер 'n'\n";
	ss << "WIN (Accept) : Строка успешно разобрана\n";
	ss << "n   (Goto)   : В какое состояние переходим после свертки нетерминала\n";
}

size_t CalculateStateWidth(size_t stateCount)
{
	size_t maxWidth = 5;
	maxWidth = std::max(maxWidth, std::to_string(stateCount > 0 ? stateCount - 1 : 0).length());
	return maxWidth;
}

std::unordered_map<std::string, size_t> CalculateNonTerminalWidths(
	const std::vector<std::string>& nonTerminals,
	const std::vector<std::unordered_map<std::string, size_t>>& gotoTable)
{
	std::unordered_map<std::string, size_t> widths;
	for (const auto& nt : nonTerminals)
	{
		size_t maxWidth = nt.length();
		for (const auto& row : gotoTable)
		{
			if (row.contains(nt))
			{
				maxWidth = std::max(maxWidth, std::to_string(row.at(nt)).length());
			}
		}
		widths[nt] = maxWidth;
	}
	return widths;
}

std::unordered_map<std::string, size_t> CalculateTerminalWidths(
	const std::vector<std::string>& terminals,
	const std::vector<std::unordered_map<std::string, LalrAction>>& actionTable,
	const LalrTable& table)
{
	std::unordered_map<std::string, size_t> widths;
	for (const auto& term : terminals)
	{
		size_t maxWidth = term.length();
		for (const auto& row : actionTable)
		{
			if (row.contains(term))
			{
				maxWidth = std::max(maxWidth, FormatAction(row.at(term), table).length());
			}
		}
		widths[term] = maxWidth;
	}
	return widths;
}

size_t CalculateEndSymbolWidth(
	const std::vector<std::unordered_map<std::string, LalrAction>>& actionTable,
	const LalrTable& table)
{
	size_t maxWidth = END_SYMBOL.length();
	for (const auto& row : actionTable)
	{
		if (row.contains(END_SYMBOL))
		{
			maxWidth = std::max(maxWidth, FormatAction(row.at(END_SYMBOL), table).length());
		}
	}
	return maxWidth;
}

size_t CalculateTotalWidth(
	size_t stateWidth,
	const std::vector<std::string>& nonTerminals,
	const std::vector<std::string>& terminals,
	const std::unordered_map<std::string, size_t>& ntWidths,
	const std::unordered_map<std::string, size_t>& termWidths,
	bool hasEndSymbol,
	size_t endSymbolWidth)
{
	size_t total = 1;
	total += stateWidth + 3;

	for (const auto& nt : nonTerminals)
	{
		total += ntWidths.at(nt) + 3;
	}

	for (const auto& term : terminals)
	{
		total += termWidths.at(term) + 3;
	}

	if (hasEndSymbol)
	{
		total += endSymbolWidth + 3;
	}

	return total;
}
} // namespace

void LalrTablePrinter::Print(const LalrTable& table)
{
	std::ostringstream ss;
	ss << "LALR(1) Таблица разбора\n";

	PrintLegend(ss);

	const std::vector<std::string> terminals = ExtractOrderedSymbols(table, true);
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

	const size_t stateWidth = CalculateStateWidth(table.actionTable.size());
	const auto ntWidths = CalculateNonTerminalWidths(nonTerminals, table.gotoTable);
	const auto termWidths = CalculateTerminalWidths(terminals, table.actionTable, table);
	const size_t endSymbolWidth = hasEndSymbol ? CalculateEndSymbolWidth(table.actionTable, table) : 0;

	const size_t totalWidth = CalculateTotalWidth(
		stateWidth, nonTerminals, terminals, ntWidths, termWidths, hasEndSymbol, endSymbolWidth);

	PrintRowSeparator(ss, totalWidth);

	ss << "|";
	PrintCell(ss, "State", stateWidth);

	for (const auto& nt : nonTerminals)
	{
		PrintCell(ss, nt, ntWidths.at(nt));
	}
	for (const auto& term : terminals)
	{
		PrintCell(ss, term, termWidths.at(term));
	}
	if (hasEndSymbol)
	{
		PrintCell(ss, END_SYMBOL, endSymbolWidth);
	}
	ss << "\n";

	PrintRowSeparator(ss, totalWidth);

	for (size_t i = 0; i < table.actionTable.size(); ++i)
	{
		ss << "|";
		PrintCell(ss, std::to_string(i), stateWidth);

		for (const auto& nt : nonTerminals)
		{
			if (table.gotoTable[i].contains(nt))
			{
				PrintCell(ss, std::to_string(table.gotoTable[i].at(nt)), ntWidths.at(nt));
			}
			else
			{
				PrintCell(ss, "", ntWidths.at(nt));
			}
		}

		for (const auto& term : terminals)
		{
			if (table.actionTable[i].contains(term))
			{
				PrintCell(ss, FormatAction(table.actionTable[i].at(term), table), termWidths.at(term));
			}
			else
			{
				PrintCell(ss, "", termWidths.at(term));
			}
		}

		if (hasEndSymbol)
		{
			if (table.actionTable[i].contains(END_SYMBOL))
			{
				PrintCell(ss, FormatAction(table.actionTable[i].at(END_SYMBOL), table), endSymbolWidth);
			}
			else
			{
				PrintCell(ss, "", endSymbolWidth);
			}
		}
		ss << "\n";
	}

	PrintRowSeparator(ss, totalWidth);

	Logger::Log(ss.str());
}