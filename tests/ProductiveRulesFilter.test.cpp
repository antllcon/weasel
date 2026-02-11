#include "src/grammar/ProductiveRulesFilter.h"
#include "src/grammar/GrammarTypes.h"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace testing;

namespace
{
    raw::Rule MakeRule(const std::string& name, const raw::Alternatives& alts)
    {
        return raw::Rule{name, alts};
    }
}

// Проверка сохранения полностью продуктивной грамматики
TEST(ProductiveRulesFilterTest, KeepsProductiveGrammar)
{
    raw::Rules input = {
        MakeRule("<S>", {{"a", "<A>"}, {"b"}}),
        MakeRule("<A>", {{"c"}})
    };
    ProductiveRulesFilter filter(std::move(input));

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].alternatives.size(), 2);
    EXPECT_EQ(result[1].alternatives.size(), 1);
}

// Проверка удаления полностью непродуктивного правила
TEST(ProductiveRulesFilterTest, RemovesUnproductiveRule)
{
    raw::Rules input = {
        MakeRule("<S>", {{"a"}}),
        MakeRule("<B>", {{"<B>"}})
    };
    ProductiveRulesFilter filter(std::move(input));

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "<S>");
}

// Проверка удаления альтернативы с непродуктивным нетерминалом
TEST(ProductiveRulesFilterTest, RemovesUnproductiveAlternative)
{
    raw::Rules input = {
        MakeRule("<S>", {{"a"}, {"<B>"}}),
        MakeRule("<B>", {{"<B>"}})
    };
    ProductiveRulesFilter filter(std::move(input));

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_EQ(result[0].alternatives[0][0], "a");
}

// Проверка корректной обработки пустого символа как продуктивного
TEST(ProductiveRulesFilterTest, TreatsEpsilonAsProductive)
{
    raw::Rules input = {
        MakeRule("<S>", {{"<A>"}}),
        MakeRule("<A>", {{EMPTY_SYMBOL}})
    };
    ProductiveRulesFilter filter(std::move(input));

    raw::Rules result = filter.FilterUnproductiveRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[1].alternatives[0][0], EMPTY_SYMBOL);
}

// Проверка выброса исключения при непродуктивном стартовом нетерминале
TEST(ProductiveRulesFilterTest, ThrowsOnUnproductiveStart)
{
    raw::Rules input = {
        MakeRule("<S>", {{"<S>"}})
    };
    ProductiveRulesFilter filter(std::move(input));

    EXPECT_THROW(
        {
            (void)filter.FilterUnproductiveRules();
        },
        std::runtime_error
    );
}