#include "src/grammar/productiveRulesFilter/ProductiveRulesFilter.h"
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

    bool HasAlternative(const raw::Rule& rule, const raw::Alternative& target)
    {
        return std::ranges::find(rule.alternatives, target) != rule.alternatives.end();
    }
}

// Проверка сохранения полностью продуктивной грамматики
TEST(ProductiveRulesFilterTest, KeepsProductiveGrammar)
{
    raw::Rules input = {
        MakeRule("S", {{"A", "b"}}),
        MakeRule("A", {{"a"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[1].name, "A");
}

// Проверка удаления непродуктивного правила и зависящих от него альтернатив
TEST(ProductiveRulesFilterTest, RemovesUnproductiveRules)
{
    raw::Rules input = {
        MakeRule("S", {{"A", "b"}, {"c"}}),
        MakeRule("A", {{"B"}}),
        MakeRule("B", {{"B"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"c"}));
}

// Проверка обработки пустых переходов как продуктивных
TEST(ProductiveRulesFilterTest, TreatsEmptySymbolAsProductive)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}}),
        MakeRule("A", {{EMPTY_SYMBOL}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(HasAlternative(result[1], {EMPTY_SYMBOL}));
}

// Проверка каскадного удаления непродуктивных правил
TEST(ProductiveRulesFilterTest, ResolvesTransitiveUnproductiveRules)
{
    raw::Rules input = {
        MakeRule("S", {{"X"}, {"a"}}),
        MakeRule("X", {{"Y"}}),
        MakeRule("Y", {{"Z"}}),
        MakeRule("Z", {{"Z", "b"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"a"}));
}

// Проверка удаления альтернатив, содержащих хотя бы один непродуктивный нетерминал
TEST(ProductiveRulesFilterTest, PrunesAlternativesWithUnproductiveSymbols)
{
    raw::Rules input = {
        MakeRule("S", {{"A", "B"}, {"a", "S"}, {"b"}}),
        MakeRule("A", {{"a"}}),
        MakeRule("B", {{"B", "c"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 2);

    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 2);
    EXPECT_TRUE(HasAlternative(result[0], {"a", "S"}));
    EXPECT_TRUE(HasAlternative(result[0], {"b"}));

    EXPECT_EQ(result[1].name, "A");
    EXPECT_EQ(result[1].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[1], {"a"}));
}

// Проверка удаления циклов, состоящих только из нетерминалов без выхода к терминалам
TEST(ProductiveRulesFilterTest, RemovesTerminalLessCycles)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}, {"b"}}),
        MakeRule("A", {{"B"}}),
        MakeRule("B", {{"C"}}),
        MakeRule("C", {{"A"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"b"}));
}

// Проверка удаления саморекурсии без базового случая
TEST(ProductiveRulesFilterTest, RemovesSelfRecursionWithoutBaseCase)
{
    raw::Rules input = {
        MakeRule("S", {{"a", "S", "b"}, {"c"}}),
        MakeRule("X", {{"a", "X"}, {"X", "b"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 2);
    EXPECT_TRUE(HasAlternative(result[0], {"a", "S", "b"}));
    EXPECT_TRUE(HasAlternative(result[0], {"c"}));
}

// Проверка сохранения продуктивных правил, даже если они недостижимы от стартового символа
TEST(ProductiveRulesFilterTest, KeepsProductiveButUnreachableRules)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}}),
        MakeRule("A", {{"a", "A"}}),
        MakeRule("B", {{"b"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "B");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"b"}));
}

// Проверка удаления правила при нескольких непродуктивных символах в альтернативе
TEST(ProductiveRulesFilterTest, HandlesMultipleUnproductiveSymbolsInAlternative)
{
    raw::Rules input = {
        MakeRule("S", {{"X", "Y", "Z"}, {"a"}}),
        MakeRule("X", {{"x"}}),
        MakeRule("Y", {{"y", "Y"}}),
        MakeRule("Z", {{"z"}})
    };
    ProductiveRulesFilter filter(input);

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 3);

    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"a"}));

    EXPECT_EQ(result[1].name, "X");
    EXPECT_EQ(result[2].name, "Z");
}