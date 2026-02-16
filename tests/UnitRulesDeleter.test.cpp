#include "src/grammar/GrammarTypes.h"
#include "src/grammar/UnitRulesDeleter.h"
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

// Проверка игнорирования грамматики без цепных правил
TEST(UnitRulesDeleterTest, IgnoresGrammarWithoutUnitRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "<A>"}, {"b"}}),
		MakeRule("<A>", {{"c"}})
	};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 2);
}

// Проверка удаления прямого цепного правила
TEST(UnitRulesDeleterTest, RemovesDirectUnitRule)
{
	raw::Rules input = {
		MakeRule("<S>", {{"<A>"}}),
		MakeRule("<A>", {{"a"}, {"b"}})
	};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_EQ(result[0].alternatives[0][0], "a");
	EXPECT_EQ(result[0].alternatives[1][0], "b");
}

// Проверка удаления транзитивных цепных правил (S -> A -> B -> c)
TEST(UnitRulesDeleterTest, ResolvesTransitiveUnitRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"<A>"}}),
		MakeRule("<A>", {{"<B>"}}),
		MakeRule("<B>", {{"c"}})
	};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result[0].alternatives.size(), 1);
	EXPECT_EQ(result[0].alternatives[0][0], "c");
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_EQ(result[1].alternatives[0][0], "c");
}

// Проверка удаления циклических цепных правил (A -> B, B -> A)
TEST(UnitRulesDeleterTest, ResolvesCyclicUnitRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"<A>"}}),
		MakeRule("<A>", {{"<B>"}, {"a"}}),
		MakeRule("<B>", {{"<A>"}, {"b"}})
	};
	UnitRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteUnitRules();

	EXPECT_EQ(result[1].alternatives.size(), 2);
	EXPECT_EQ(result[2].alternatives.size(), 2);
}

// Проверка выбрасывания исключения при пустой грамматике
TEST(UnitRulesDeleterTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	UnitRulesDeleter deleter(input);

	EXPECT_THROW(
		{
			(void)deleter.DeleteUnitRules();
		},
		std::runtime_error
	);
}