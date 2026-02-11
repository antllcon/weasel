#include "src/grammar/EmptyRulesDeleter.h"
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
} // namespace

// Проверка игнорирования грамматики из одного правила
TEST(EmptyRulesDeleterTest, ReturnsUnchangedForSmallGrammar)
{
	raw::Rules input = {
		MakeRule("S", {{EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].alternatives[0][0], EMPTY_SYMBOL);
}

// Проверка грамматики без пустых правил
TEST(EmptyRulesDeleterTest, NoEmptyRules)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "b"}}),
		MakeRule("A", {{"a"}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 1);
	EXPECT_EQ(result[1].alternatives.size(), 1);
}

// Проверка удаления простого пустого правила и генерации новой альтернативы
TEST(EmptyRulesDeleterTest, RemovesSingleEmptyRule)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "b"}}),
		MakeRule("A", {{"a"}, {EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_EQ(result[0].alternatives[1].size(), 1);
	EXPECT_EQ(result[0].alternatives[1][0], "b");
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_EQ(result[1].alternatives[0][0], "a");
}

// Проверка корректной замены исчезающего нетерминала на пустой символ
TEST(EmptyRulesDeleterTest, PropagatesEmptySymbol)
{
	raw::Rules input = {
		MakeRule("S", {{"A"}}),
		MakeRule("A", {{EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_EQ(result[0].alternatives[0][0], "A");
	EXPECT_EQ(result[0].alternatives[1][0], EMPTY_SYMBOL);
	EXPECT_EQ(result[1].alternatives.size(), 0);
}

// Проверка выброса исключения при наличии нескольких пустых нетерминалов в одной альтернативе
TEST(EmptyRulesDeleterTest, ThrowsOnMultipleEmptySymbols)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "A"}}),
		MakeRule("A", {{EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	EXPECT_THROW(
		{
			(void)deleter.DeleteEmptyRules();
		},
		std::runtime_error);
}