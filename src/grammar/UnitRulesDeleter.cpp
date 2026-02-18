#include "UnitRulesDeleter.h"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace
{
	using AdjacencyMap = std::unordered_map<std::string, std::unordered_set<std::string>>;
	using RuleMap = std::unordered_map<std::string, const raw::Rule*>;

	void AssertIsGrammarNotEmpty(bool isEmpty)
	{
		if (isEmpty)
		{
			throw std::runtime_error("Грамматика пуста, невозможно удалить цепные правила");
		}
	}

	bool IsUnitAlternative(const raw::Alternative& alternative)
	{
		return alternative.size() == 1 && !IsTerm(alternative.front());
	}

	AdjacencyMap BuildUnitGraph(const raw::Rules& rules)
	{
		AdjacencyMap graph;
		for (const auto& rule : rules)
		{
			for (const auto& alt : rule.alternatives)
			{
				if (IsUnitAlternative(alt))
				{
					graph[rule.name].insert(alt.front());
				}
			}
		}
		return graph;
	}

	std::unordered_set<std::string> FindReachableNonTerminals(
		const std::string& startNode,
		const AdjacencyMap& graph)
	{
		std::unordered_set<std::string> visited;
		std::queue<std::string> queue;

		queue.push(startNode);
		visited.insert(startNode);

		while (!queue.empty())
		{
			const std::string current = queue.front();
			queue.pop();

			if (graph.contains(current))
			{
				for (const auto& neighbor : graph.at(current))
				{
					if (!visited.contains(neighbor))
					{
						visited.insert(neighbor);
						queue.push(neighbor);
					}
				}
			}
		}

		return visited;
	}

	RuleMap CreateRuleLookup(const raw::Rules& rules)
	{
		RuleMap lookup;
		for (const auto& rule : rules)
		{
			lookup[rule.name] = &rule;
		}
		return lookup;
	}

	void CollectNonUnitAlternatives(
		raw::Alternatives& destination,
		const raw::Rule& sourceRule)
	{
		for (const auto& alt : sourceRule.alternatives)
		{
			if (!IsUnitAlternative(alt))
			{
				const bool exists = std::ranges::any_of(destination, [&](const auto& existing) {
					return existing == alt;
				});

				if (!exists)
				{
					destination.push_back(alt);
				}
			}
		}
	}
}

UnitRulesDeleter::UnitRulesDeleter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules UnitRulesDeleter::DeleteUnitRules()
{
	AssertIsGrammarNotEmpty(m_rules.empty());

	const AdjacencyMap unitGraph = BuildUnitGraph(m_rules);
	const RuleMap ruleLookup = CreateRuleLookup(m_rules);

	for (auto& rule : m_rules)
	{
		const std::unordered_set<std::string> reachable = FindReachableNonTerminals(rule.name, unitGraph);

		raw::Alternatives newAlternatives;

		for (const auto& reachableSymbol : reachable)
		{
			if (ruleLookup.contains(reachableSymbol))
			{
				CollectNonUnitAlternatives(newAlternatives, *ruleLookup.at(reachableSymbol));
			}
		}

		rule.alternatives = std::move(newAlternatives);
	}

	return m_rules;
}