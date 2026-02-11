#include "EmptyRulesDeleter.h"
#include <stdexcept>
#include <algorithm>
#include <optional>

namespace
{
    void AssertIsSingleEpsilonSymbol(size_t count)
    {
        if (count > 1)
        {
            throw std::runtime_error("Правило содержит более одного пустого символа для генерации");
        }
    }

    bool IsEpsilonAlternative(const raw::Alternative& alternative)
    {
        return alternative.size() == 1 && alternative.front() == EMPTY_SYMBOL;
    }

    std::optional<std::string> FindNullableRuleName(const raw::Rules& rules)
    {
        if (rules.size() < 2)
        {
            return std::nullopt;
        }

        for (size_t i = 1; i < rules.size(); ++i)
        {
            for (const auto& alternative : rules[i].alternatives)
            {
                if (IsEpsilonAlternative(alternative))
                {
                    return rules[i].name;
                }
            }
        }

        return std::nullopt;
    }

    void PropagateNullableRule(raw::Rules& rules, const std::string& emptyRuleName)
    {
        for (auto& rule : rules)
        {
            const size_t initialSize = rule.alternatives.size();
            for (size_t i = 0; i < initialSize; ++i)
            {
                const auto& alternative = rule.alternatives[i];
                const size_t emptyCount = std::ranges::count(alternative, emptyRuleName);

                AssertIsSingleEpsilonSymbol(emptyCount);

                if (emptyCount == 1)
                {
                    raw::Alternative newAlternative;
                    for (const auto& symbol : alternative)
                    {
                        if (symbol != emptyRuleName)
                        {
                            newAlternative.push_back(symbol);
                        }
                    }

                    if (newAlternative.empty())
                    {
                        newAlternative.push_back(EMPTY_SYMBOL);
                    }

                    rule.alternatives.push_back(std::move(newAlternative));
                }
            }
        }
    }

    void DeleteEpsilonAlternative(raw::Rules& rules, const std::string& ruleName)
    {
        const auto it = std::ranges::find_if(rules, [&ruleName](const raw::Rule& rule) {
            return rule.name == ruleName;
        });

        if (it != rules.end())
        {
            std::erase_if(it->alternatives, IsEpsilonAlternative);
        }
    }
}

EmptyRulesDeleter::EmptyRulesDeleter(raw::Rules rules)
    : m_rules(std::move(rules))
{
}

raw::Rules EmptyRulesDeleter::DeleteEmptyRules()
{
    if (m_rules.size() < 2)
    {
        return m_rules;
    }

    while (const auto emptyRuleName = FindNullableRuleName(m_rules))
    {
        PropagateNullableRule(m_rules, emptyRuleName.value());
        DeleteEpsilonAlternative(m_rules, emptyRuleName.value());
    }

    return m_rules;
}