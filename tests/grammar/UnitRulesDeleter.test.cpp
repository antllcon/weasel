#include "src/grammar/unitRulesDeleter/UnitRulesDeleter.h"
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
} // namespace

// Проверка удаления простого цепного правила
TEST(UnitRulesDeleterTest, RemovesSimpleUnitRule)
{
	raw::Rules input = {
		MakeRule("S", {{"A"}}),
		MakeRule("A", {{"a", "b"}})};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].name, "S");
	EXPECT_EQ(result[0].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[0], {"a", "b"}));
}

// Проверка транзитивного раскрытия нескольких цепных правил
TEST(UnitRulesDeleterTest, ResolvesTransitiveUnitRules)
{
	raw::Rules input = {
		MakeRule("S", {{"A"}}),
		MakeRule("A", {{"B"}}),
		MakeRule("B", {{"c"}})};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(HasAlternative(result[0], {"c"}));
	EXPECT_TRUE(HasAlternative(result[1], {"c"}));
	EXPECT_TRUE(HasAlternative(result[2], {"c"}));
}

// Проверка обработки циклов между нетерминалами
TEST(UnitRulesDeleterTest, HandlesCyclesBetweenNonTerminals)
{
	raw::Rules input = {
		MakeRule("A", {{"B"}, {"a"}}),
		MakeRule("B", {{"A"}, {"b"}})};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(result[0], {"a"}));
	EXPECT_TRUE(HasAlternative(result[0], {"b"}));
	EXPECT_EQ(result[1].alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(result[1], {"a"}));
	EXPECT_TRUE(HasAlternative(result[1], {"b"}));
}

// Проверка самостоятельного удаления петель (Spin Rules)
TEST(UnitRulesDeleterTest, RemovesSelfSpinRules)
{
	raw::Rules input = {
		MakeRule("S", {{"S"}, {"x"}})};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[0], {"x"}));
}

// Проверка разрешения ромбовидных зависимостей без дублирования альтернатив
TEST(UnitRulesDeleterTest, HandlesDiamondDependencies)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}, {"B"}}),
        MakeRule("A", {{"C"}}),
        MakeRule("B", {{"C"}}),
        MakeRule("C", {{"c"}})
    };
    UnitRulesDeleter deleter(input);

    raw::Rules result = deleter.DeleteUnitRules();

    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"c"}));
}

// Проверка отличия цепных правил от правил с терминалами или сложными альтернативами
TEST(UnitRulesDeleterTest, IgnoresNonUnitSingletonsAndComplexAlternatives)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}, {"A", "b"}, {"c"}}),
        MakeRule("A", {{"a"}})
    };
    UnitRulesDeleter deleter(input);

    raw::Rules result = deleter.DeleteUnitRules();

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 3);
    EXPECT_TRUE(HasAlternative(result[0], {"a"}));
    EXPECT_TRUE(HasAlternative(result[0], {"A", "b"}));
    EXPECT_TRUE(HasAlternative(result[0], {"c"}));
}

// Проверка длинной цепочки с вложенными петлями на себя
TEST(UnitRulesDeleterTest, HandlesLongChainsWithSelfLoops)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}}),
        MakeRule("A", {{"A"}, {"B"}}),
        MakeRule("B", {{"C"}, {"B"}}),
        MakeRule("C", {{"x"}, {"y"}})
    };
    UnitRulesDeleter deleter(input);

    raw::Rules result = deleter.DeleteUnitRules();

    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 2);
    EXPECT_TRUE(HasAlternative(result[0], {"x"}));
    EXPECT_TRUE(HasAlternative(result[0], {"y"}));
}

// Проверка корректного удаления правил, состоящих исключительно из бесконечных цепных переходов
TEST(UnitRulesDeleterTest, RemovesRulesWithoutNonUnitResolutions)
{
    raw::Rules input = {
        MakeRule("S", {{"A"}, {"x"}}),
        MakeRule("A", {{"B"}}),
        MakeRule("B", {{"A"}, {"C"}}),
        MakeRule("C", {{"C"}})
    };
    UnitRulesDeleter deleter(input);

    raw::Rules result = deleter.DeleteUnitRules();

    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "S");
    EXPECT_EQ(result[0].alternatives.size(), 1);
    EXPECT_TRUE(HasAlternative(result[0], {"x"}));
}