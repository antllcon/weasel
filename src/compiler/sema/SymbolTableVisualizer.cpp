#include "SymbolTableVisualizer.h"
#include "src/utils/logger/Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
using SortedSymbols = std::vector<std::pair<std::string, SymbolInfo>>;

void AssertIsNotEmpty(const std::unordered_map<std::string, SymbolInfo>& symbols)
{
	if (symbols.empty())
	{
		throw std::runtime_error("Таблица символов пуста");
	}
}

SortedSymbols SortBySlot(const std::unordered_map<std::string, SymbolInfo>& symbols)
{
	SortedSymbols sorted(symbols.begin(), symbols.end());
	std::ranges::sort(sorted, [](const auto& a, const auto& b) {
		return a.second.stackSlot < b.second.stackSlot;
	});
	return sorted;
}

std::string GetTypeName(const SymbolInfo& info)
{
	return info.type ? info.type->GetName() : "unknown";
}

std::string GetMutability(const SymbolInfo& info)
{
	return info.isMutable ? "var" : "val/def";
}

void AppendHeader(std::ostringstream& ss)
{
	ss << "[Sema]\t\tТаблица символов:" << std::endl;
	ss << "\t\t| "
	   << std::left << std::setw(20) << "Name" << " | "
	   << std::setw(12) << "Slot" << " | "
	   << std::setw(12) << "Mutable" << " | "
	   << std::setw(12) << "Type" << " |" << std::endl;
}

void AppendRow(std::ostringstream& ss, const std::string& name, const SymbolInfo& info)
{
	ss << "\t\t| "
	   << std::left << std::setw(20) << name << " | "
	   << std::setw(12) << info.stackSlot << " | "
	   << std::setw(12) << GetMutability(info) << " | "
	   << std::setw(12) << GetTypeName(info) << " |" << std::endl;
}

std::string BuildTable(const SortedSymbols& symbols)
{
	std::ostringstream ss;
	AppendHeader(ss);
	for (const auto& [name, info] : symbols)
	{
		AppendRow(ss, name, info);
	}
	return ss.str();
}
} // namespace

void SymbolTableVisualizer::Visualize(const std::unordered_map<std::string, SymbolInfo>& symbols)
{
	AssertIsNotEmpty(symbols);
	auto sorted = SortBySlot(symbols);
	Logger::Log(BuildTable(sorted));
}