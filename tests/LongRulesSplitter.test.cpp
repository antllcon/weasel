#include "src/grammar/GrammarTypes.h"
#include "src/grammar/LongRulesSplitter.h"
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

// Проверка игнорирования грамматики, где все правила имеют длину 1 или 2
TEST(LongRulesSplitterTest, IgnoresShortRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "<A>"}, {"b"}}),
		MakeRule("<A>", {{"c", "d"}})
	};
	LongRulesSplitter splitter(input);

	raw::Rules result = splitter.SplitLongRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives[0].size(), 2);
	EXPECT_EQ(result[0].alternatives[1].size(), 1);
	EXPECT_EQ(result[1].alternatives[0].size(), 2);
}

// Проверка корректного разбиения правила из трех символов
TEST(LongRulesSplitterTest, SplitsThreeSymbolRule)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "b", "c"}})
	};
	LongRulesSplitter splitter(input);

	raw::Rules result = splitter.SplitLongRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].name, "<S>");
	EXPECT_EQ(result[0].alternatives[0].size(), 2);
	EXPECT_EQ(result[0].alternatives[0][0], "a");
	EXPECT_EQ(result[0].alternatives[0][1], "<N1>");
	EXPECT_EQ(result[1].name, "<N1>");
	EXPECT_EQ(result[1].alternatives[0].size(), 2);
	EXPECT_EQ(result[1].alternatives[0][0], "b");
	EXPECT_EQ(result[1].alternatives[0][1], "c");
}

// Проверка корректного разбиения правила из четырех символов
TEST(LongRulesSplitterTest, SplitsFourSymbolRule)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "b", "c", "d"}})
	};
	LongRulesSplitter splitter(input);

	raw::Rules result = splitter.SplitLongRules();

	EXPECT_EQ(result.size(), 3);
	EXPECT_EQ(result[0].alternatives[0][1], "<N1>");
	EXPECT_EQ(result[1].name, "<N1>");
	EXPECT_EQ(result[1].alternatives[0][1], "<N2>");
	EXPECT_EQ(result[2].name, "<N2>");
	EXPECT_EQ(result[2].alternatives[0].size(), 2);
	EXPECT_EQ(result[2].alternatives[0][0], "c");
	EXPECT_EQ(result[2].alternatives[0][1], "d");
}

// Проверка выбрасывания исключения при пустой грамматике
TEST(LongRulesSplitterTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	LongRulesSplitter splitter(input);

	EXPECT_THROW(
		{
			(void)splitter.SplitLongRules();
		},
		std::runtime_error
	);
}