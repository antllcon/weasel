#include "GrammarConsistencyChecker.h"
#include <stdexcept>
#include <unordered_set>

namespace
{
void AssertIsNonTerminalDefined(bool isDefined, const std::string& name)
{
	if (!isDefined)
	{
		throw std::runtime_error("Нетерминал '" + name + "' используется, но не имеет правил");
	}
}
} // namespace

void GrammarConsistencyChecker::Check(const raw::Rules& rules)
{
	std::unordered_set<std::string> definedNonTerminals;

	for (const auto& [name, alternatives] : rules)
	{
		definedNonTerminals.insert(name);
	}

	for (const auto& [name, alternatives] : rules)
	{
		for (const auto& alt : alternatives)
		{
			for (const auto& symbol : alt)
			{
				if (!IsTerm(symbol))
				{
					AssertIsNonTerminalDefined(definedNonTerminals.contains(symbol), symbol);
				}
			}
		}
	}
}