#include "src/grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "src/grammar/GrammarTypes.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{
	raw::Rule MakeRawRule(const std::string& name, const raw::Alternatives& alts)
	{
		return raw::Rule{name, alts};
	}

	bool HasGuide(const Rule& rule, size_t altIndex, const std::string& term)
	{
		return rule.alternatives.at(altIndex).guides.contains(term);
	}
}

// Проверка вычисления направляющих множеств только по First для простых правил
TEST(GuideSetsCalculatorTest, CalculatesSimpleFirstSets)
{
	raw::Rules input = {
		MakeRawRule("S", {{"a", "A"}, {"b"}}),
		MakeRawRule("A", {{"c"}})
	};
	GuideSetsCalculator calculator(input, "S");

	Rules result = calculator.Calculate();

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(HasGuide(result[0], 0, "a"));
	EXPECT_TRUE(HasGuide(result[0], 1, "b"));
	EXPECT_TRUE(HasGuide(result[1], 0, "c"));
}

// Проверка подмешивания Follow множеств при наличии пустых правил
TEST(GuideSetsCalculatorTest, CalculatesFollowSetsForEmptyRules)
{
	raw::Rules input = {
		MakeRawRule("S", {{"A", "a"}, {"B", "b"}}),
		MakeRawRule("A", {{"c"}, {EMPTY_SYMBOL}}),
		MakeRawRule("B", {{EMPTY_SYMBOL}})
	};
	GuideSetsCalculator calculator(input, "S");

	Rules result = calculator.Calculate();

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(HasGuide(result[1], 0, "c"));
	EXPECT_TRUE(HasGuide(result[1], 1, "a"));
	EXPECT_TRUE(HasGuide(result[2], 0, "b"));
}

// Проверка обработки стартового символа и маркера конца строки
TEST(GuideSetsCalculatorTest, AddsEndSymbolToStartRuleFollowSet)
{
	raw::Rules input = {
		MakeRawRule("S", {{"A"}}),
		MakeRawRule("A", {{EMPTY_SYMBOL}})
	};
	GuideSetsCalculator calculator(input, "S");

	Rules result = calculator.Calculate();

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(HasGuide(result[1], 0, END_SYMBOL));
}

// Проверка выброса исключения при пустой грамматике
TEST(GuideSetsCalculatorTest, ThrowsOnEmptyGrammar)
{
	raw::Rules input;
	GuideSetsCalculator calculator(input, "S");

	EXPECT_THROW((void)calculator.Calculate(), std::runtime_error);
}