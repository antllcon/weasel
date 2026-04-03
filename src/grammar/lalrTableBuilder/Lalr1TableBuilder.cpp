#include "Lalr1TableBuilder.h"
#include <algorithm>
#include <queue>
#include <stdexcept>

namespace
{
using FirstSets = std::unordered_map<std::string, std::unordered_set<std::string>>;

void AssertIsGrammarNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Грамматика пуста, построение LALR(1) таблицы невозможно");
	}
}

void AssertIsConflictFree(bool hasConflict, const std::string& conflictType, const std::string& symbol)
{
	if (hasConflict)
	{
		throw std::runtime_error("Обнаружен конфликт " + conflictType + " по символу '" + symbol + "'");
	}
}

raw::Rules AugmentGrammar(const raw::Rules& originalRules, const std::string& startSymbol)
{
	raw::Rules rules = originalRules;
	raw::Rule startRule;
	startRule.name = "Z'";
	startRule.alternatives.push_back({startSymbol});
	rules.insert(rules.begin(), std::move(startRule));
	return rules;
}

FirstSets CalculateFirstSets(const raw::Rules& rules)
{
	FirstSets firstSets;
	for (const auto& [name, alternatives] : rules)
	{
		firstSets[name] = {};
	}

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& [name, alternatives] : rules)
		{
			for (const auto& alt : alternatives)
			{
				bool allEmpty = true;
				for (const auto& symbol : alt)
				{
					if (IsTerm(symbol))
					{
						if (firstSets[name].insert(symbol).second)
						{
							changed = true;
						}
						allEmpty = false;
						break;
					}

					if (symbol == EMPTY_SYMBOL)
					{
						continue;
					}

					const auto& symbolFirst = firstSets[symbol];
					for (const auto& term : symbolFirst)
					{
						if (term != EMPTY_SYMBOL)
						{
							if (firstSets[name].insert(term).second)
							{
								changed = true;
							}
						}
					}

					if (!symbolFirst.contains(EMPTY_SYMBOL))
					{
						allEmpty = false;
						break;
					}
				}

				if (allEmpty || (alt.size() == 1 && alt.front() == EMPTY_SYMBOL))
				{
					if (firstSets[name].insert(EMPTY_SYMBOL).second)
					{
						changed = true;
					}
				}
			}
		}
	}
	return firstSets;
}

std::unordered_set<std::string> GetSequenceFirst(
	const std::vector<std::string>& sequence,
	size_t startIndex,
	const std::string& lookahead,
	const FirstSets& firstSets)
{
	std::unordered_set<std::string> result;
	bool allContainEmpty = true;

	for (size_t i = startIndex; i < sequence.size(); ++i)
	{
		const std::string& symbol = sequence[i];
		if (IsTerm(symbol))
		{
			result.insert(symbol);
			allContainEmpty = false;
			break;
		}

		if (symbol == EMPTY_SYMBOL)
		{
			continue;
		}

		const auto& symbolFirst = firstSets.at(symbol);
		for (const auto& term : symbolFirst)
		{
			if (term != EMPTY_SYMBOL)
			{
				result.insert(term);
			}
		}

		if (!symbolFirst.contains(EMPTY_SYMBOL))
		{
			allContainEmpty = false;
			break;
		}
	}

	if (allContainEmpty)
	{
		result.insert(lookahead);
	}

	return result;
}

std::vector<LalrItem> ComputeClosure(std::vector<LalrItem> items, const raw::Rules& rules, const FirstSets& firstSets)
{
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (size_t i = 0; i < items.size(); ++i)
		{
			Lr0Item core = items[i].core;
			std::unordered_set<std::string> lookaheads = items[i].lookaheads;

			const auto& alt = rules[core.ruleIndex].alternatives[core.altIndex];

			if (core.dotPos >= alt.size())
			{
				continue;
			}

			const std::string& symbolAfterDot = alt[core.dotPos];
			if (IsTerm(symbolAfterDot))
			{
				continue;
			}

			for (const auto& lookahead : lookaheads)
			{
				std::unordered_set<std::string> firstOfRest = GetSequenceFirst(alt, core.dotPos + 1, lookahead, firstSets);

				for (size_t r = 0; r < rules.size(); ++r)
				{
					if (rules[r].name == symbolAfterDot)
					{
						for (size_t a = 0; a < rules[r].alternatives.size(); ++a)
						{
							const bool isEmptyAlt = rules[r].alternatives[a].size() == 1 && rules[r].alternatives[a].front() == EMPTY_SYMBOL;
							Lr0Item newCore{r, a, isEmptyAlt ? 1ull : 0ull};

							auto it = std::ranges::find_if(items, [&](const LalrItem& current) {
								return current.core == newCore;
							});

							if (it != items.end())
							{
								for (const auto& f : firstOfRest)
								{
									if (it->lookaheads.insert(f).second)
									{
										changed = true;
									}
								}
							}
							else
							{
								LalrItem newItem;
								newItem.core = newCore;
								newItem.lookaheads = firstOfRest;
								items.push_back(std::move(newItem));
								changed = true;
							}
						}
					}
				}
			}
		}
	}
	return items;
}

std::vector<LalrItem> ComputeGoto(const std::vector<LalrItem>& stateItems, const std::string& symbol, const raw::Rules& rules, const FirstSets& firstSets)
{
	std::vector<LalrItem> nextItems;
	for (const auto& item : stateItems)
	{
		const auto& alt = rules[item.core.ruleIndex].alternatives[item.core.altIndex];
		if (item.core.dotPos < alt.size() && alt[item.core.dotPos] == symbol)
		{
			LalrItem nextItem = item;
			nextItem.core.dotPos++;
			nextItems.push_back(std::move(nextItem));
		}
	}
	return ComputeClosure(nextItems, rules, firstSets);
}

bool AreCoresEqual(const std::vector<LalrItem>& a, const std::vector<LalrItem>& b)
{
	if (a.size() != b.size())
	{
		return false;
	}

	for (const auto& itemA : a)
	{
		auto it = std::ranges::find_if(b, [&](const LalrItem& itemB) {
			return itemB.core == itemA.core;
		});

		if (it == b.end())
		{
			return false;
		}
	}
	return true;
}

bool MergeLookaheads(std::vector<LalrItem>& dest, const std::vector<LalrItem>& src)
{
	bool changed = false;
	for (const auto& srcItem : src)
	{
		auto it = std::ranges::find_if(dest, [&](const LalrItem& current) {
			return current.core == srcItem.core;
		});

		if (it != dest.end())
		{
			for (const auto& lookahead : srcItem.lookaheads)
			{
				if (it->lookaheads.insert(lookahead).second)
				{
					changed = true;
				}
			}
		}
	}
	return changed;
}

void AddAction(std::unordered_map<std::string, LalrAction>& row, const std::string& symbol, const LalrAction& newAction)
{
	if (row.contains(symbol))
	{
		const auto& existing = row.at(symbol);
		if (existing.type == LalrActionType::Shift && newAction.type == LalrActionType::Reduce)
		{
			AssertIsConflictFree(true, "Shift-Reduce", symbol);
		}
		if (existing.type == LalrActionType::Reduce && newAction.type == LalrActionType::Shift)
		{
			AssertIsConflictFree(true, "Shift-Reduce", symbol);
		}
		if (existing.type == LalrActionType::Reduce && newAction.type == LalrActionType::Reduce && (existing.ruleIndex != newAction.ruleIndex || existing.altIndex != newAction.altIndex))
		{
			AssertIsConflictFree(true, "Reduce-Reduce", symbol);
		}
	}
	row[symbol] = newAction;
}

std::unordered_set<std::string> ExtractGrammarSymbols(const raw::Rules& rules)
{
	std::unordered_set<std::string> symbols;
	for (const auto& [name, alternatives] : rules)
	{
		symbols.insert(name);
		for (const auto& alt : alternatives)
		{
			for (const auto& symbol : alt)
			{
				if (symbol != EMPTY_SYMBOL)
				{
					symbols.insert(symbol);
				}
			}
		}
	}
	return symbols;
}
} // namespace

Lalr1TableBuilder::Lalr1TableBuilder(raw::Rules rules, std::string startSymbol)
	: m_rules(std::move(rules))
	, m_startSymbol(std::move(startSymbol))
{
}

LalrTable Lalr1TableBuilder::Build() const
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	raw::Rules rules = AugmentGrammar(m_rules, m_startSymbol);
	FirstSets firstSets = CalculateFirstSets(rules);
	std::unordered_set<std::string> symbols = ExtractGrammarSymbols(rules);

	std::vector<LalrState> states;
	std::queue<size_t> worklist;

	LalrItem initialItem;
	initialItem.core = {0, 0, 0};
	initialItem.lookaheads.insert(END_SYMBOL);

	LalrState initialState;
	initialState.index = 0;
	initialState.items = ComputeClosure({initialItem}, rules, firstSets);
	states.push_back(std::move(initialState));
	worklist.push(0);

	while (!worklist.empty())
	{
		const size_t currentStateIndex = worklist.front();
		worklist.pop();

		for (const auto& symbol : symbols)
		{
			std::vector<LalrItem> nextItems = ComputeGoto(states[currentStateIndex].items, symbol, rules, firstSets);
			if (nextItems.empty())
			{
				continue;
			}

			auto existingStateIt = std::ranges::find_if(states, [&](const LalrState& s) {
				return AreCoresEqual(s.items, nextItems);
			});

			if (existingStateIt != states.end())
			{
				if (MergeLookaheads(existingStateIt->items, nextItems))
				{
					worklist.push(existingStateIt->index);
				}
				states[currentStateIndex].transitions[symbol] = existingStateIt->index;
			}
			else
			{
				LalrState newState;
				newState.index = states.size();
				newState.items = std::move(nextItems);
				states.push_back(std::move(newState));

				worklist.push(newState.index);
				states[currentStateIndex].transitions[symbol] = newState.index;
			}
		}
	}

	LalrTable table;
	table.actionTable.resize(states.size());
	table.gotoTable.resize(states.size());
	table.augmentedRules = rules;

	for (const auto& [index, items, transitions] : states)
	{
		for (const auto& [core, lookaheads] : items)
		{
			const auto& alt = rules[core.ruleIndex].alternatives[core.altIndex];
			const bool isComplete = core.dotPos == alt.size() || (alt.size() == 1 && alt.front() == EMPTY_SYMBOL && core.dotPos == 1);

			if (isComplete)
			{
				if (core.ruleIndex == 0)
				{
					AddAction(table.actionTable[index], END_SYMBOL, {LalrActionType::Accept, 0, 0});
				}
				else
				{
					for (const auto& lookahead : lookaheads)
					{
						AddAction(table.actionTable[index], lookahead, {LalrActionType::Reduce, core.ruleIndex, core.altIndex});
					}
				}
			}
		}

		for (const auto& [symbol, nextState] : transitions)
		{
			if (IsTerm(symbol))
			{
				AddAction(table.actionTable[index], symbol, {LalrActionType::Shift, nextState, 0});
			}
			else
			{
				table.gotoTable[index][symbol] = nextState;
			}
		}
	}

	return table;
}