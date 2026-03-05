#include "src/grammar/llValidator/LlValidator.h"
#include "src/grammar/GrammarTypes.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{
Rule MakeRuleWithGuides(const std::string& name, const std::vector<Guides>& guidesList)
{
	Rule rule;
	rule.name = name;
	for (const auto& guides : guidesList)
	{
		Alternative alt;
		alt.guides = guides;
		rule.alternatives.push_back(std::move(alt));
	}
	return rule;
}
}

// Проверка валидации грамматики с непересекающимися направляющими множествами
TEST(LlValidatorTest, ReturnsTrueForValidGrammar)
{
	Rules input = {
		MakeRuleWithGuides("S", {{"a", "b"}, {"c"}}),
		MakeRuleWithGuides("A", {{"d"}, {"e", "f"}})
	};
	LlValidator validator(input);

	EXPECT_TRUE(validator.IsValid());
}

// Проверка отклонения грамматики при пересечении направляющих множеств в одном правиле
TEST(LlValidatorTest, ReturnsFalseForIntersectingGuides)
{
	Rules input = {
		MakeRuleWithGuides("S", {{"a", "b"}, {"b", "c"}})
	};
	LlValidator validator(input);

	EXPECT_FALSE(validator.IsValid());
}

// Проверка успешной валидации правил с единственной альтернативой
TEST(LlValidatorTest, ReturnsTrueForSingleAlternatives)
{
	Rules input = {
		MakeRuleWithGuides("S", {{"a", "b", "c"}})
	};
	LlValidator validator(input);

	EXPECT_TRUE(validator.IsValid());
}

// Проверка выброса исключения при пустой грамматике
TEST(LlValidatorTest, ThrowsOnEmptyGrammar)
{
	Rules input;
	LlValidator validator(input);

	EXPECT_THROW((void)validator.IsValid(), std::runtime_error);
}