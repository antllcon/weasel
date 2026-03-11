#include "Ll1TablePrinter.h"
#include <iomanip>
#include <iostream>

namespace
{
constexpr int COL_INDEX = 8;
constexpr int COL_NAME = 8;
constexpr int COL_GUIDES = 16;
constexpr int COL_ERROR = 8;
constexpr int COL_TRANS = 8;
constexpr int COL_SHIFT = 8;
constexpr int COL_STACK = 8;

std::string BoolToStr(bool value)
{
	return value ? "yes" : "no";
}

std::string FormatTransition(const std::optional<size_t>& transition)
{
	if (transition)
	{
		return std::to_string(*transition);
	}
	return "null";
}

void PrintSeparator()
{
	std::cout
		<< std::string(COL_INDEX + COL_NAME + COL_GUIDES + COL_ERROR + COL_TRANS + COL_SHIFT + COL_STACK + 7 * 3 + 1, '-')
		<< std::endl;
}

void PrintCell(const std::string& text, int width)
{
	std::cout << " " << std::left << std::setw(width) << text << " |";
}
}

void Ll1TablePrinter::Print(const Ll1Table& table)
{
	std::cout << "Таблица разбора" << std::endl;

	PrintSeparator();

	std::cout << "|";
	PrintCell("№     ", COL_INDEX);
	PrintCell("Rule ", COL_NAME);
	PrintCell("Guide", COL_GUIDES);
	PrintCell("Error", COL_ERROR);
	PrintCell("Trans", COL_TRANS);
	PrintCell("Shift", COL_SHIFT);
	PrintCell("Stack", COL_STACK);
	std::cout << std::endl;

	PrintSeparator();

	for (const auto& row : table)
	{
		std::cout << "|";

		PrintCell(std::to_string(row.index), COL_INDEX);
		PrintCell(row.name, COL_NAME);
		PrintCell(row.guides, COL_GUIDES);
		PrintCell(BoolToStr(row.error), COL_ERROR);
		PrintCell(FormatTransition(row.transition), COL_TRANS);
		PrintCell(BoolToStr(row.shift), COL_SHIFT);
		PrintCell(BoolToStr(row.stack), COL_STACK);

		std::cout << std::endl;
	}

	PrintSeparator();
}