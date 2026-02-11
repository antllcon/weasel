#include "ProductiveRulesFilter.h"
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <ranges>

namespace
{
    void AssertIsStartSymbolProductive(bool isProductive)
    {
        if (!isProductive)
        {
            throw std::runtime_error("Стартовый нетерминал является непродуктивным");
        }
    }

    bool IsSymbolProductive(const std::string& symbol, const std::unordered_set<std::string>& productiveSet)
    {
        return IsTerm(symbol) || productiveSet.contains(symbol) || symbol == EMPTY_SYMBOL;
    }

    bool IsAlternativeProductive(const raw::Alternative& alternative, const std::unordered_set<std::string>& productiveSet)
    {
        return std::ranges::all_of(alternative, [&productiveSet](const std::string& symbol) {
            return IsSymbolProductive(symbol, productiveSet);
        });
    }

    std::unordered_set<std::string> FindProductiveNonTerminals(const raw::Rules& rules)
    {
        std::unordered_set<std::string> productiveSet;
        bool changed = true;

        while (changed)
        {
            changed = false;
            for (const auto& rule : rules)
            {
                if (productiveSet.contains(rule.name))
                {
                    continue;
                }

                for (const auto& alternative : rule.alternatives)
                {
                    if (IsAlternativeProductive(alternative, productiveSet))
                    {
                        productiveSet.insert(rule.name);
                        changed = true;
                        break;
                    }
                }
            }
        }

        return productiveSet;
    }

    void CleanUnproductiveAlternatives(raw::Rule& rule, const std::unordered_set<std::string>& productiveSet)
    {
        std::erase_if(rule.alternatives, [&productiveSet](const raw::Alternative& alternative) {
            return !IsAlternativeProductive(alternative, productiveSet);
        });
    }
}

ProductiveRulesFilter::ProductiveRulesFilter(raw::Rules rules)
    : m_rules(std::move(rules))
{
}

raw::Rules ProductiveRulesFilter::FilterUnproductiveRules()
{
    if (m_rules.empty())
    {
        return m_rules;
    }

    std::unordered_set<std::string> productiveSet = FindProductiveNonTerminals(m_rules);

    AssertIsStartSymbolProductive(productiveSet.contains(m_rules.front().name));

    raw::Rules filteredRules;
    for (auto rule : m_rules)
    {
        if (productiveSet.contains(rule.name))
        {
            CleanUnproductiveAlternatives(rule, productiveSet);

            if (!rule.alternatives.empty())
            {
                filteredRules.push_back(std::move(rule));
            }
        }
    }

    return filteredRules;
}