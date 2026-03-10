#include "Ll1TablePrinter.h"
#include <iomanip>
#include <iostream>

namespace
{
constexpr int COL_INDEX = 4;
constexpr int COL_GUIDE = 30;
constexpr int COL_SHIFT = 6;
constexpr int COL_TRANS = 12;
constexpr int COL_STACK = 8;
constexpr int COL_ERROR = 8;
constexpr int COL_FINAL = 8;

std::string BoolToStr(bool value)
{
	return value ? "yes" : "no";
}

std::string FormatTransition(const std::optional<size_t>& transition)
{
	if (transition)
		return std::to_string(*transition);
	return "stak";
}

void PrintSeparator()
{
	std::cout
		<< "+"
		<< std::string(COL_INDEX + 2, '-')
		<< "+"
		<< std::string(COL_GUIDE + 2, '-')
		<< "+"
		<< std::string(COL_SHIFT + 2, '-')
		<< "+"
		<< std::string(COL_TRANS + 2, '-')
		<< "+"
		<< std::string(COL_STACK + 2, '-')
		<< "+"
		<< std::string(COL_ERROR + 2, '-')
		<< "+"
		<< std::string(COL_FINAL + 2, '-')
		<< "+" << std::endl;
}

void PrintCell(const std::string& text, int width)
{
	std::cout << " " << std::left << std::setw(width) << text << " |";
}
} // namespace

void Ll1TablePrinter::Print(const Ll1Table& table)
{
	std::cout << "Таблица LL(1) разбора:" << std::endl;

	PrintSeparator();

	std::cout << "|";
	PrintCell("№   ", COL_INDEX);
	PrintCell("Guide", COL_GUIDE);
	PrintCell("Shift", COL_SHIFT);
	PrintCell("Trans", COL_TRANS);
	PrintCell("Stack", COL_STACK);
	PrintCell("Error", COL_ERROR);
	PrintCell("Final", COL_FINAL);
	std::cout << std::endl;

	PrintSeparator();

	for (const auto& row : table)
	{
		std::cout << "|";

		PrintCell(std::to_string(row.index), COL_INDEX);
		PrintCell(row.guideSet, COL_GUIDE);
		PrintCell(BoolToStr(row.shift), COL_SHIFT);
		PrintCell(FormatTransition(row.transition), COL_TRANS);
		PrintCell(BoolToStr(row.stack), COL_STACK);
		PrintCell(BoolToStr(row.error), COL_ERROR);
		PrintCell(BoolToStr(row.finalState), COL_FINAL);

		std::cout << std::endl;
	}

	PrintSeparator();
}