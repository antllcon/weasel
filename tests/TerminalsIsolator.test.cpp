#include "src/grammar/GrammarTypes.h"
#include "src/grammar/TerminalsIsolator.h"
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

// Проверка игнорирования одиночных терминалов
TEST(TerminalsIsolatorTest, IgnoresSingleTerminals)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a"}, {"b"}})
	};
	TerminalsIsolator isolator(input);

	raw::Rules result = isolator.IsolateTerminals();

	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result[0].alternatives[0][0], "a");
	EXPECT_EQ(result[0].alternatives[1][0], "b");
}

// Проверка изоляции терминалов в смешанных правилах
TEST(TerminalsIsolatorTest, IsolatesMixedRules)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "<B>"}}),
		MakeRule("<B>", {{"b", "c"}})
	};
	TerminalsIsolator isolator(input);

	raw::Rules result = isolator.IsolateTerminals();

	EXPECT_EQ(result.size(), 4);
	EXPECT_EQ(result[0].alternatives[0][0], "<T_a>");
	EXPECT_EQ(result[0].alternatives[0][1], "<B>");
	EXPECT_EQ(result[1].alternatives[0][0], "<T_b>");
	EXPECT_EQ(result[1].alternatives[0][1], "<T_c>");
	EXPECT_EQ(result[2].name, "<T_a>");
	EXPECT_EQ(result[3].name, "<T_b>");
}

// Проверка переиспользования созданных нетерминалов для одинаковых терминалов
TEST(TerminalsIsolatorTest, ReusesTerminalWrappers)
{
	raw::Rules input = {
		MakeRule("<S>", {{"a", "<B>"}}),
		MakeRule("<B>", {{"a", "<C>"}}),
		MakeRule("<C>", {{"c"}})
	};
	TerminalsIsolator isolator(input);

	raw::Rules result = isolator.IsolateTerminals();

	EXPECT_EQ(result.size(), 4);
	EXPECT_EQ(result[0].alternatives[0][0], "<T_a>");
	EXPECT_EQ(result[1].alternatives[0][0], "<T_a>");
	EXPECT_EQ(result[3].name, "<T_a>");
}

// Проверка выбрасывания исключения при пустой грамматике
TEST(TerminalsIsolatorTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	TerminalsIsolator isolator(input);

	EXPECT_THROW(
		{
			(void)isolator.IsolateTerminals();
		},
		std::runtime_error
	);
}