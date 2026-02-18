#include "CykLogger.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
	void AssertIsTableNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Таблица разбора пуста, логирование невозможно");
		}
	}

	std::string FormatCell(const CykCell& cell)
	{
		if (cell.empty())
		{
			return "-";
		}

		std::vector<std::string> sorted(cell.begin(), cell.end());
		std::ranges::sort(sorted);

		std::string result = "{";
		for (size_t i = 0; i < sorted.size(); ++i)
		{
			result += sorted[i];
			if (i + 1 < sorted.size())
			{
				result += ",";
			}
		}
		result += "}";
		return result;
	}

	size_t CalculateCellWidth(const CykTable& table, size_t length)
	{
		size_t maxWidth = 1;
		for (size_t l = 1; l <= length; ++l)
		{
			for (size_t i = 0; i <= length - l; ++i)
			{
				maxWidth = std::max(maxWidth, FormatCell(table[i][l]).size());
			}
		}

		maxWidth += 2;

		if (maxWidth % 2 != 0)
		{
			maxWidth++;
		}

		return maxWidth;
	}

	std::string CenterString(const std::string& text, size_t width)
	{
		if (text.length() >= width)
		{
			return text;
		}
		const size_t padding = width - text.length();
		const size_t leftPad = padding / 2;
		const size_t rightPad = padding - leftPad;
		return std::string(leftPad, ' ') + text + std::string(rightPad, ' ');
	}
}

void CykLogger::LogPyramid(const CykTable& table, const std::string& word)
{
	AssertIsTableNotEmpty(word.empty() || table.empty());

	const size_t length = word.size();
	const size_t cellWidth = CalculateCellWidth(table, length);

	std::cout << std::endl;

	for (size_t l = length; l >= 1; --l)
	{
		const size_t offset = (l - 1) * (cellWidth / 2);
		std::cout << std::string(offset, ' ');

		for (size_t i = 0; i <= length - l; ++i)
		{
			std::cout << CenterString(FormatCell(table[i][l]), cellWidth);
		}
		std::cout << std::endl;
	}

	for (size_t i = 0; i < length; ++i)
	{
		const std::string charStr(1, word[i]);
		std::cout << CenterString(charStr, cellWidth);
	}

	std::cout << std::endl << std::endl;
}