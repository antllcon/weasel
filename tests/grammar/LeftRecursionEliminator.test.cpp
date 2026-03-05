#include "src/grammar/leftRecursionEliminator/LeftRecursionEliminator.h"
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

	const raw::Rule* FindRule(const raw::Rules& rules, const std::string& name)
	{
		auto it = std::ranges::find_if(rules, [&name](const raw::Rule& rule) {
			return rule.name == name;
		});
		return it != rules.end() ? &(*it) : nullptr;
	}
}

// Проверка сохранения грамматики без левой рекурсии
TEST(LeftRecursionEliminatorTest, KeepsGrammarWithoutLeftRecursion)
{
	raw::Rules input = {
		MakeRule("S", {{"a", "A"}, {"b"}}),
		MakeRule("A", {{"c", "A"}, {"d"}})
	};
	LeftRecursionEliminator eliminator(input);

	raw::Rules result = eliminator.Eliminate();

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(HasAlternative(result[0], {"a", "A"}));
	EXPECT_TRUE(HasAlternative(result[1], {"c", "A"}));
}

// Проверка устранения прямой левой рекурсии
TEST(LeftRecursionEliminatorTest, EliminatesDirectLeftRecursion)
{
	raw::Rules input = {
		MakeRule("E", {{"E", "+", "T"}, {"T"}})
	};
	LeftRecursionEliminator eliminator(input);

	raw::Rules result = eliminator.Eliminate();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* eRule = FindRule(result, "E");
	ASSERT_NE(eRule, nullptr);
	EXPECT_TRUE(HasAlternative(*eRule, {"T", "E'"}));

	const raw::Rule* ePrimeRule = FindRule(result, "E'");
	ASSERT_NE(ePrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*ePrimeRule, {"+", "T", "E'"}));
	EXPECT_TRUE(HasAlternative(*ePrimeRule, {EMPTY_SYMBOL}));
}

// Проверка устранения косвенной левой рекурсии
TEST(LeftRecursionEliminatorTest, EliminatesIndirectLeftRecursion)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "a"}, {"b"}}),
		MakeRule("A", {{"A", "c"}, {"S", "d"}, {"e"}})
	};
	LeftRecursionEliminator eliminator(input);

	raw::Rules result = eliminator.Eliminate();

	EXPECT_EQ(result.size(), 3);

	const raw::Rule* aPrimeRule = FindRule(result, "A'");
	ASSERT_NE(aPrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"c", "A'"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"a", "d", "A'"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {EMPTY_SYMBOL}));
}

// Проверка обработки правила, полностью состоящего из леворекурсивных альтернатив
TEST(LeftRecursionEliminatorTest, HandlesOnlyLeftRecursiveAlternatives)
{
	raw::Rules input = {
		MakeRule("S", {{"S", "a"}, {"S", "b"}})
	};
	LeftRecursionEliminator eliminator(input);

	raw::Rules result = eliminator.Eliminate();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* sRule = FindRule(result, "S");
	ASSERT_NE(sRule, nullptr);
	EXPECT_TRUE(HasAlternative(*sRule, {"S'"}));

	const raw::Rule* sPrimeRule = FindRule(result, "S'");
	ASSERT_NE(sPrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*sPrimeRule, {"a", "S'"}));
	EXPECT_TRUE(HasAlternative(*sPrimeRule, {"b", "S'"}));
	EXPECT_TRUE(HasAlternative(*sPrimeRule, {EMPTY_SYMBOL}));
}

// Проверка выброса исключения при пустой грамматике
TEST(LeftRecursionEliminatorTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	LeftRecursionEliminator eliminator(input);

	EXPECT_THROW((void)eliminator.Eliminate(), std::runtime_error);
}

// Проверка точного совпадения с теоретическим примером из лекции
TEST(LeftRecursionEliminatorTest, SolvesLectureExampleExactly)
{
    raw::Rules input = {
        MakeRule("A", {{"S", "a"}}),
        MakeRule("S", {{"S", "b"}, {"A", "c"}, {"b"}})
    };
    LeftRecursionEliminator eliminator(input);

    raw::Rules result = eliminator.Eliminate();

    EXPECT_EQ(result.size(), 3);

    const raw::Rule* aRule = FindRule(result, "A");
    ASSERT_NE(aRule, nullptr);
    EXPECT_TRUE(HasAlternative(*aRule, {"S", "a"}));

    const raw::Rule* sRule = FindRule(result, "S");
    ASSERT_NE(sRule, nullptr);
    EXPECT_TRUE(HasAlternative(*sRule, {"b", "S'"}));

    const raw::Rule* sPrimeRule = FindRule(result, "S'");
    ASSERT_NE(sPrimeRule, nullptr);
    EXPECT_TRUE(HasAlternative(*sPrimeRule, {"b", "S'"}));
    EXPECT_TRUE(HasAlternative(*sPrimeRule, {"a", "c", "S'"}));
    EXPECT_TRUE(HasAlternative(*sPrimeRule, {EMPTY_SYMBOL}));
}

// Проверка устранения множественной прямой рекурсии (несколько альф и бет)
TEST(LeftRecursionEliminatorTest, HandlesMultipleDirectRecursions)
{
    raw::Rules input = {
        MakeRule("E", {{"E", "+", "T"}, {"E", "-", "T"}, {"T"}, {"F"}})
    };
    LeftRecursionEliminator eliminator(input);

    raw::Rules result = eliminator.Eliminate();

    EXPECT_EQ(result.size(), 2);

    const raw::Rule* eRule = FindRule(result, "E");
    ASSERT_NE(eRule, nullptr);
    EXPECT_TRUE(HasAlternative(*eRule, {"T", "E'"}));
    EXPECT_TRUE(HasAlternative(*eRule, {"F", "E'"}));

    const raw::Rule* ePrimeRule = FindRule(result, "E'");
    ASSERT_NE(ePrimeRule, nullptr);
    EXPECT_TRUE(HasAlternative(*ePrimeRule, {"+", "T", "E'"}));
    EXPECT_TRUE(HasAlternative(*ePrimeRule, {"-", "T", "E'"}));
    EXPECT_TRUE(HasAlternative(*ePrimeRule, {EMPTY_SYMBOL}));
}

// Проверка раскрытия глубокой цепочки косвенной рекурсии (3 уровня)
TEST(LeftRecursionEliminatorTest, ResolvesDeepIndirectRecursionChain)
{
	raw::Rules input = {
		MakeRule("A", {{"B", "a"}, {"x"}}),
		MakeRule("B", {{"C", "b"}, {"y"}}),
		MakeRule("C", {{"A", "c"}, {"z"}})
	};
	LeftRecursionEliminator eliminator(input);

	raw::Rules result = eliminator.Eliminate();

	EXPECT_EQ(result.size(), 4);

	const raw::Rule* aRule = FindRule(result, "A");
	ASSERT_NE(aRule, nullptr);
	EXPECT_TRUE(HasAlternative(*aRule, {"B", "a"}));
	EXPECT_TRUE(HasAlternative(*aRule, {"x"}));

	const raw::Rule* cRule = FindRule(result, "C");
	ASSERT_NE(cRule, nullptr);
	EXPECT_TRUE(HasAlternative(*cRule, {"z", "C'"}));
	EXPECT_TRUE(HasAlternative(*cRule, {"y", "a", "c", "C'"}));
	EXPECT_TRUE(HasAlternative(*cRule, {"x", "c", "C'"}));

	const raw::Rule* cPrimeRule = FindRule(result, "C'");
	ASSERT_NE(cPrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*cPrimeRule, {"b", "a", "c", "C'"}));
	EXPECT_TRUE(HasAlternative(*cPrimeRule, {EMPTY_SYMBOL}));
}

// Проверка худшего случая разрастания грамматики (без зацикливания)
TEST(LeftRecursionEliminatorTest, HandlesExponentialGrowthWithoutHanging)
{
    raw::Rules input = {
        MakeRule("A1", {{"0"}, {"1"}}),
        MakeRule("A2", {{"A1", "0"}, {"A1", "1"}}),
        MakeRule("A3", {{"A2", "0"}, {"A2", "1"}})
    };
    LeftRecursionEliminator eliminator(input);

    raw::Rules result = eliminator.Eliminate();

    EXPECT_EQ(result.size(), 3);

    const raw::Rule* a3Rule = FindRule(result, "A3");
    ASSERT_NE(a3Rule, nullptr);
    EXPECT_EQ(a3Rule->alternatives.size(), 8);
    EXPECT_TRUE(HasAlternative(*a3Rule, {"0", "0", "0"}));
    EXPECT_TRUE(HasAlternative(*a3Rule, {"1", "1", "1"}));
}