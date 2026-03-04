#include "src/grammar/reachableRulesFilter/ReachableRulesFilter.h"
#include "src/grammar/GrammarTypes.h"
#include <algorithm>
#include <gtest/gtest.h>

using namespace testing;

namespace
{
    raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
    {
        return raw::Rule{name, alts};
    }

    bool HasRule(const raw::Rules& rules, const std::string& ruleName)
    {
        return std::ranges::any_of(rules, [&ruleName](const raw::Rule& rule) {
            return rule.name == ruleName;
        });
    }
}

// Проверка сохранения полностью достижимой грамматики
TEST(ReachableRulesFilterTest, KeepsFullyReachableGrammar)
{
    raw::Rules input = {
        MakeRule("S", {{"A", "b"}}),
        MakeRule("A", {{"a"}})
    };
    ReachableRulesFilter filter(input, "S");

    raw::Rules result = filter.FilterUnreachableRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(HasRule(result, "S"));
    EXPECT_TRUE(HasRule(result, "A"));
}

// Проверка удаления полностью изолированных правил
TEST(ReachableRulesFilterTest, RemovesIsolatedRules)
{
    raw::Rules input = {
        MakeRule("S", {{"a"}}),
        MakeRule("B", {{"b"}})
    };
    ReachableRulesFilter filter(input, "S");

    raw::Rules result = filter.FilterUnreachableRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(HasRule(result, "S"));
    EXPECT_FALSE(HasRule(result, "B"));
}

// Проверка удаления графа правил, отсоединенного от стартового символа
TEST(ReachableRulesFilterTest, RemovesDisconnectedSubgraphs)
{
    raw::Rules input = {
        MakeRule("S", {{"a"}}),
        MakeRule("X", {{"Y"}}),
        MakeRule("Y", {{"Z"}}),
        MakeRule("Z", {{"z"}})
    };
    ReachableRulesFilter filter(input, "S");

    raw::Rules result = filter.FilterUnreachableRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(HasRule(result, "S"));
    EXPECT_FALSE(HasRule(result, "X"));
    EXPECT_FALSE(HasRule(result, "Y"));
    EXPECT_FALSE(HasRule(result, "Z"));
}

// Проверка обработки циклов в достижимых и недостижимых правилах
TEST(ReachableRulesFilterTest, HandlesCyclesCorrectly)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}}),
        MakeRule("A", {{"S", "a"}}),
        MakeRule("X", {{"Y"}}),
        MakeRule("Y", {{"X", "b"}})
    };
    ReachableRulesFilter filter(input, "S");

    raw::Rules result = filter.FilterUnreachableRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(HasRule(result, "S"));
    EXPECT_TRUE(HasRule(result, "A"));
    EXPECT_FALSE(HasRule(result, "X"));
    EXPECT_FALSE(HasRule(result, "Y"));
}

// Проверка исключения при отсутствии стартового символа
TEST(ReachableRulesFilterTest, ThrowsWhenStartSymbolIsMissing)
{
	raw::Rules input = {
		MakeRule("A", {{"a"}})
	};
	ReachableRulesFilter filter(input, "S");

	EXPECT_THROW((void)filter.FilterUnreachableRules(), std::runtime_error);
}