#include "src/grammar/emptyRulesDeleter/EmptyRulesDeleter.h"
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

// Проверка сохранения грамматики без пустых правил
TEST(EmptyRulesDeleterTest, KeepsGrammarWithoutEmptyRules)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "b"}}),
		MakeRule("A", {{"a"}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 1);
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[0], {"A", "b"}));
}

// Проверка удаления простого пустого правила и генерации новой альтернативы
TEST(EmptyRulesDeleterTest, RemovesSingleEmptyRule)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "b"}, {"c"}}),
		MakeRule("A", {{"a"}, {EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 3);
	EXPECT_TRUE(HasAlternative(result[0], {"A", "b"}));
	EXPECT_TRUE(HasAlternative(result[0], {"b"}));
	EXPECT_TRUE(HasAlternative(result[0], {"c"}));
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[1], {"a"}));
}

// Проверка транзитивного удаления пустых правил
// Ожидаемое поведение: B удаляется (так как там только 'e')
// A остается с правилом {"B"}, так как чистка непродуктивных правил - отдельный этап
TEST(EmptyRulesDeleterTest, ResolvesTransitiveNullableRules)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "b"}}),
		MakeRule("A", {{"B"}}),
		MakeRule("B", {{EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);

	EXPECT_EQ(result[0].name, "S");
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(result[0], {"A", "b"}));
	EXPECT_TRUE(HasAlternative(result[0], {"b"}));

	EXPECT_EQ(result[1].name, "A");
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[1], {"B"}));
}

// Проверка генерации всех комбинаций для нескольких обнуляемых нетерминалов
TEST(EmptyRulesDeleterTest, GeneratesCombinationsForMultipleNullables)
{
	raw::Rules input = {
		MakeRule("S", {{"A", "B", "c"}}),
		MakeRule("A", {{"a"}, {EMPTY_SYMBOL}}),
		MakeRule("B", {{"b"}, {EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 3);
	EXPECT_EQ(result[0].alternatives.size(), 4);
	EXPECT_TRUE(HasAlternative(result[0], {"A", "B", "c"}));
	EXPECT_TRUE(HasAlternative(result[0], {"A", "c"}));
	EXPECT_TRUE(HasAlternative(result[0], {"B", "c"}));
	EXPECT_TRUE(HasAlternative(result[0], {"c"}));
}

// Проверка сохранения пустого символа для стартового нетерминала
TEST(EmptyRulesDeleterTest, PreservesEmptySymbolForStartRule)
{
	raw::Rules input = {
		MakeRule("S", {{"A"}}),
		MakeRule("A", {{"a"}, {EMPTY_SYMBOL}})};
	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(result[0], {"A"}));
	EXPECT_TRUE(HasAlternative(result[0], {EMPTY_SYMBOL}));
	EXPECT_EQ(result[1].alternatives.size(), 1);
	EXPECT_TRUE(HasAlternative(result[1], {"a"}));
}

// Проверка обработки правила полностью состоящего из обнуляемых нетерминалов
// Ожидаемое поведение: A и B удалятся.
// Y останется со всеми комбинациями {"A", "B"}, {"A"} и {"B"}, так как они не пустые
// Полное удаление Y произойдет только на этапе удаления непродуктивных правил
TEST(EmptyRulesDeleterTest, HandlesFullyNullableAlternative)
{
	raw::Rules input = {
		MakeRule("S", {{"x", "Y"}}),
		MakeRule("Y", {{"A", "B"}}),
		MakeRule("A", {{EMPTY_SYMBOL}}),
		MakeRule("B", {{EMPTY_SYMBOL}})};

	EmptyRulesDeleter deleter(input);

	raw::Rules result = deleter.DeleteEmptyRules();

	// Остаются S и Y
	EXPECT_EQ(result.size(), 2);

	EXPECT_EQ(result[0].name, "S");
	EXPECT_EQ(result[0].alternatives.size(), 2);
	EXPECT_TRUE(HasAlternative(result[0], {"x", "Y"}));
	EXPECT_TRUE(HasAlternative(result[0], {"x"}));

	EXPECT_EQ(result[1].name, "Y");
	EXPECT_EQ(result[1].alternatives.size(), 3);
	EXPECT_TRUE(HasAlternative(result[1], {"A", "B"}));
	EXPECT_TRUE(HasAlternative(result[1], {"A"}));
	EXPECT_TRUE(HasAlternative(result[1], {"B"}));
}