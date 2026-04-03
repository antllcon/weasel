#pragma once
#include "src/grammar/GrammarTypes.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class LalrActionType
{
	Shift,
	Reduce,
	Accept,
	Error
};

struct LalrAction
{
	LalrActionType type;
	size_t ruleIndex;
	size_t altIndex;
};

struct Lr0Item
{
	size_t ruleIndex;
	size_t altIndex;
	size_t dotPos;

	bool operator==(const Lr0Item&) const = default;
};

struct Lr0ItemHasher
{
	size_t operator()(const Lr0Item& item) const
	{
		return std::hash<size_t>()(item.ruleIndex) ^ std::hash<size_t>()(item.altIndex) << 1 ^ (std::hash<size_t>()(item.dotPos) << 2);
	}
};

struct LalrItem
{
	Lr0Item core;
	std::unordered_set<std::string> lookaheads;
};

struct LalrState
{
	size_t index;
	std::vector<LalrItem> items;
	std::unordered_map<std::string, size_t> transitions;
};

struct LalrTable
{
	std::vector<std::unordered_map<std::string, LalrAction>> actionTable;
	std::vector<std::unordered_map<std::string, size_t>> gotoTable;
	raw::Rules augmentedRules;
};