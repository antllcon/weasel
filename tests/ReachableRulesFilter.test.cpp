#include "src/grammar/ReachableRulesFilter.h"
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

// Проверка сохранения полностью достижимой грамматики
TEST(ReachableRulesFilterTest, KeepsReachableGrammar)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "<A>"}}),
		MakeRule("<A>", {{"b"}})
	};
	ReachableRulesFilter filter(std::move(input));

	raw::Rules result = filter.FilterUnreachableRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].name, "<S>");
	EXPECT_EQ(result[1].name, "<A>");
}

// Проверка удаления простого недостижимого правила
TEST(ReachableRulesFilterTest, RemovesUnreachableRule)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a"}}),
		MakeRule("<B>", {{"b"}})
	};
	ReachableRulesFilter filter(std::move(input));

	raw::Rules result = filter.FilterUnreachableRules();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].name, "<S>");
}

// Проверка удаления недостижимых правил с циклическими зависимостями друг на друга
TEST(ReachableRulesFilterTest, RemovesUnreachableCyclicRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a"}}),
		MakeRule("<B>", {{"<C>"}}),
		MakeRule("<C>", {{"<B>"}})
	};
	ReachableRulesFilter filter(std::move(input));

	raw::Rules result = filter.FilterUnreachableRules();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].name, "<S>");
}

// Проверка выброса исключения при попытке фильтрации пустой грамматики
TEST(ReachableRulesFilterTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	ReachableRulesFilter filter(std::move(input));

	EXPECT_THROW(
		{
			(void)filter.FilterUnreachableRules();
		},
		std::runtime_error
	);
}