#pragma once
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class ActionType;
inline const std::string END_SYMBOL = "#";
inline const std::string EMPTY_SYMBOL = "e";

namespace raw
{
using Alternative = std::vector<std::string>;
using Alternatives = std::vector<Alternative>;

struct Rule
{
	std::string name;
	Alternatives alternatives;
};

using Rules = std::vector<Rule>;
} // namespace raw

using Guides = std::unordered_set<std::string>;

struct Alternative
{
	std::vector<std::string> rule;
	Guides guides;
};

using Alternatives = std::vector<Alternative>;

struct Rule
{
	std::string name;
	Alternatives alternatives;
};

using Rules = std::vector<Rule>;

struct GrammarEntry
{
	size_t rule;
	size_t pos;

	bool operator==(const GrammarEntry& other) const = default;

	struct Hasher
	{
		size_t operator()(const GrammarEntry& entry) const
		{
			return std::hash<size_t>()(entry.rule) ^ std::hash<size_t>()(entry.pos);
		}
	};
};

using GrammarEntriesSet = std::unordered_set<GrammarEntry, GrammarEntry::Hasher>;

struct Action
{
	ActionType type;
	size_t value;
	std::string ruleName;
	size_t ruleSize = 0;
	bool isOk = false;

	bool operator==(const Action&) const = default;
};

using TableRow = std::unordered_map<std::string, Action>;
using Table = std::vector<TableRow>;

inline bool IsTerm(const std::string& term)
{
	if (term.empty())
	{
		return false;
	}

	if (std::isupper(static_cast<unsigned char>(term[0])))
	{
		return false;
	}

	return true;
}

using CykCell = std::unordered_set<std::string>;
using CykRow = std::vector<CykCell>;
using CykTable = std::vector<CykRow>;

struct CykParseResult
{
	bool isBelongsToLanguage;
	CykTable table;
};