#include "GrammarOptimizer.h"
#include "ProductiveRulesFilter.h"
#include "ReachableRulesFilter.h"
#include <algorithm>
#include <stdexcept>

namespace
{
void AssertIsStartSymbolPresent(const raw::Rules& rules, std::string_view startSymbol)
{
	const bool isPresent = std::ranges::any_of(rules, [startSymbol](const raw::Rule& rule) {
		return rule.name == startSymbol;
	});

	if (!isPresent)
	{
		throw std::runtime_error("Стартовый символ не найден в грамматике");
	}
}
} // namespace

namespace GrammarOptimizer
{
raw::Rules OptimizeForLalr(raw::Rules rules, std::string_view startSymbol)
{
	AssertIsStartSymbolPresent(rules, startSymbol);

	rules = ProductiveRulesFilter(std::move(rules)).FilterUnproductiveRules();
	rules = ReachableRulesFilter(std::move(rules), std::string(startSymbol)).FilterUnreachableRules();

	return rules;
}

raw::Rules AugmentGrammarLalr(raw::Rules rules, std::string_view startSymbol, std::string& outNewStart)
{
	outNewStart = "Z";
	while (std::ranges::any_of(rules, [&](const raw::Rule& r) { return r.name == outNewStart; }))
	{
		outNewStart += "'";
	}

	raw::Rule rootRule;
	rootRule.name = outNewStart;
	rootRule.alternatives.push_back({std::string(startSymbol)});

	rules.insert(rules.begin(), std::move(rootRule));
	return rules;
}
} // namespace GrammarOptimizer
