#include "src/grammar/leftFactorizer/LeftFactorizer.h"
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

// Проверка сохранения грамматики, которая уже факторизована
TEST(LeftFactorizerTest, KeepsAlreadyFactorizedGrammar)
{
	raw::Rules input = {
		MakeRule("S", {{"a", "B"}, {"c", "D"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].alternatives.size(), 2);
}

// Проверка простой левой факторизации одного префикса
TEST(LeftFactorizerTest, HandlesSimpleFactorization)
{
	raw::Rules input = {
		MakeRule("A", {{"a", "b"}, {"a", "c"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* aRule = FindRule(result, "A");
	ASSERT_NE(aRule, nullptr);
	EXPECT_EQ(aRule->alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(*aRule, {"a", "A'"}));

	const raw::Rule* aPrimeRule = FindRule(result, "A'");
	ASSERT_NE(aPrimeRule, nullptr);
	EXPECT_EQ(aPrimeRule->alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"b"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"c"}));
}

// Проверка факторизации, когда префикс совпадает с одной из альтернатив целиком
TEST(LeftFactorizerTest, HandlesFactorizationWithEmptySuffix)
{
	raw::Rules input = {
		MakeRule("A", {{"a"}, {"a", "b"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* aRule = FindRule(result, "A");
	ASSERT_NE(aRule, nullptr);
	EXPECT_TRUE(HasAlternative(*aRule, {"a", "A'"}));

	const raw::Rule* aPrimeRule = FindRule(result, "A'");
	ASSERT_NE(aPrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {EMPTY_SYMBOL}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"b"}));
}

// Проверка многоуровневой (глубокой) факторизации (классический if-then-else)
TEST(LeftFactorizerTest, HandlesDeepFactorization)
{
	raw::Rules input = {
		MakeRule("S", {{"i", "E", "t", "S"}, {"i", "E", "t", "S", "else", "S"}, {"a"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* sRule = FindRule(result, "S");
	ASSERT_NE(sRule, nullptr);
	EXPECT_TRUE(HasAlternative(*sRule, {"i", "E", "t", "S", "S'"}));
	EXPECT_TRUE(HasAlternative(*sRule, {"a"}));

	const raw::Rule* sPrimeRule = FindRule(result, "S'");
	ASSERT_NE(sPrimeRule, nullptr);
	EXPECT_TRUE(HasAlternative(*sPrimeRule, {EMPTY_SYMBOL}));
	EXPECT_TRUE(HasAlternative(*sPrimeRule, {"else", "S"}));
}

// Проверка нескольких разных независимых групп префиксов в одном правиле
TEST(LeftFactorizerTest, HandlesMultipleFactorizationGroups)
{
	raw::Rules input = {
		MakeRule("A", {{"x", "1"}, {"x", "2"}, {"y", "1"}, {"y", "2"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 3);

	const raw::Rule* aRule = FindRule(result, "A");
	ASSERT_NE(aRule, nullptr);
	EXPECT_TRUE(HasAlternative(*aRule, {"x", "A'"}));
	EXPECT_TRUE(HasAlternative(*aRule, {"y", "A''"}));
}

// Проверка каскадной факторизации (префикс внутри префикса)
TEST(LeftFactorizerTest, HandlesCascadingPrefixes)
{
	raw::Rules input = {
		MakeRule("A", {{"a", "b", "c"}, {"a", "b", "d"}, {"a", "x"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	// Жадный алгоритм сначала выносит самый длинный префикс "a b" в A',
	// а затем оставшийся префикс "a" в A''.
	// Ожидаем:
	// A -> a A''
	// A' -> c | d
	// A'' -> b A' | x
	EXPECT_EQ(result.size(), 3);

	const raw::Rule* aRule = FindRule(result, "A");
	EXPECT_TRUE(HasAlternative(*aRule, {"a", "A''"}));

	const raw::Rule* aPrimeRule = FindRule(result, "A'");
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"c"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"d"}));

	const raw::Rule* aDoublePrimeRule = FindRule(result, "A''");
	EXPECT_TRUE(HasAlternative(*aDoublePrimeRule, {"b", "A'"}));
	EXPECT_TRUE(HasAlternative(*aDoublePrimeRule, {"x"}));
}

// Проверка объединения более чем двух альтернатив с одинаковым префиксом
TEST(LeftFactorizerTest, FactorsMultipleAlternativesAtOnce)
{
	raw::Rules input = {
		MakeRule("A", {{"a", "1"}, {"a", "2"}, {"a", "3"}})
	};
	LeftFactorizer factorizer(input);

	raw::Rules result = factorizer.Factorize();

	EXPECT_EQ(result.size(), 2);

	const raw::Rule* aRule = FindRule(result, "A");
	EXPECT_EQ(aRule->alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(*aRule, {"a", "A'"}));

	const raw::Rule* aPrimeRule = FindRule(result, "A'");
	EXPECT_EQ(aPrimeRule->alternatives.size(), 3);
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"1"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"2"}));
	EXPECT_TRUE(HasAlternative(*aPrimeRule, {"3"}));
}