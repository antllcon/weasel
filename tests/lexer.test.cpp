#include "src/compiler/lexer/Lexer.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include <gtest/gtest.h>
#include <string_view>

using namespace testing;

// Проверка выбрасывания исключения при пустом вводе
TEST(LexerTest, EmptyInputThrows)
{
	DiagnosticEngine engine;
	std::string_view input = "";

	EXPECT_ANY_THROW(Lexer::Tokenize(input, engine));
}

// Проверка распознавания базовых типов токенов: идентификаторов и разделителей
TEST(LexerTest, BasicTokens)
{
	DiagnosticEngine engine;
	std::string_view input = "ident , . : { }";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 7);
	EXPECT_EQ(tokens[0].type, TokenType::Identifier);
	EXPECT_EQ(tokens[1].type, TokenType::Comma);
	EXPECT_EQ(tokens[2].type, TokenType::Dot);
	EXPECT_EQ(tokens[3].type, TokenType::Colon);
	EXPECT_EQ(tokens[4].type, TokenType::BraceLeft);
	EXPECT_EQ(tokens[5].type, TokenType::BraceRight);
	EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

// Проверка отличия ключевых слов от идентификаторов с совпадающим префиксом
TEST(LexerTest, KeywordVsIdentifier)
{
	DiagnosticEngine engine;
	std::string_view input = "val valx values";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::Keyword);
	EXPECT_EQ(tokens[0].value, "val");
	EXPECT_EQ(tokens[1].type, TokenType::Identifier);
	EXPECT_EQ(tokens[1].value, "valx");
	EXPECT_EQ(tokens[2].type, TokenType::Identifier);
	EXPECT_EQ(tokens[2].value, "values");
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка парсинга целых чисел
TEST(LexerTest, IntegerToken)
{
	DiagnosticEngine engine;
	std::string_view input = "123 0 99";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::Integer);
	EXPECT_EQ(tokens[0].value, "123");
	EXPECT_EQ(tokens[1].type, TokenType::Integer);
	EXPECT_EQ(tokens[1].value, "0");
	EXPECT_EQ(tokens[2].type, TokenType::Integer);
	EXPECT_EQ(tokens[2].value, "99");
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка парсинга чисел с плавающей точкой
TEST(LexerTest, FloatToken)
{
	DiagnosticEngine engine;
	std::string_view input = "1.0 3.14";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 3);
	EXPECT_EQ(tokens[0].type, TokenType::Float);
	EXPECT_EQ(tokens[0].value, "1.0");
	EXPECT_EQ(tokens[1].type, TokenType::Float);
	EXPECT_EQ(tokens[1].value, "3.14");
	EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

// Проверка корректного разрешения конфликтов между целыми числами, числами с точкой и оператором диапазона
TEST(LexerTest, IntegerVsFloatVsRange)
{
	DiagnosticEngine engine;
	std::string_view input = "1 1.0 1..2";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 6);
	EXPECT_EQ(tokens[0].type, TokenType::Integer);
	EXPECT_EQ(tokens[0].value, "1");
	EXPECT_EQ(tokens[1].type, TokenType::Float);
	EXPECT_EQ(tokens[1].value, "1.0");
	EXPECT_EQ(tokens[2].type, TokenType::Integer);
	EXPECT_EQ(tokens[2].value, "1");
	EXPECT_EQ(tokens[3].type, TokenType::OpRange);
	EXPECT_EQ(tokens[4].type, TokenType::Integer);
	EXPECT_EQ(tokens[4].value, "2");
	EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

// Проверка парсинга строк, включая пустые строки
TEST(LexerTest, StringToken)
{
	DiagnosticEngine engine;
	std::string_view input = "\"hello\" \"\"";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 3);
	EXPECT_EQ(tokens[0].type, TokenType::String);
	EXPECT_EQ(tokens[0].value, "\"hello\"");
	EXPECT_EQ(tokens[1].type, TokenType::String);
	EXPECT_EQ(tokens[1].value, "\"\"");
	EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

// Проверка генерации нефатальной ошибки при незакрытой строке
TEST(LexerTest, UnclosedStringError)
{
	DiagnosticEngine engine;
	std::string_view input = "\"unclosed";

	auto tokens = Lexer::Tokenize(input, engine);

	EXPECT_TRUE(engine.HasErrors());
	EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

// Проверка приоритета многосимвольных операторов над односимвольными
TEST(LexerTest, OperatorPriority)
{
	DiagnosticEngine engine;
	std::string_view input = ": := < <- . .. < << >> > >= > >< ==";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 15);
	EXPECT_EQ(tokens[0].type, TokenType::Colon);
	EXPECT_EQ(tokens[1].type, TokenType::OpAssign);
	EXPECT_EQ(tokens[2].type, TokenType::OpLess);
	EXPECT_EQ(tokens[3].type, TokenType::OpMove);
	EXPECT_EQ(tokens[4].type, TokenType::Dot);
	EXPECT_EQ(tokens[5].type, TokenType::OpRange);
	EXPECT_EQ(tokens[6].type, TokenType::OpLess);
	EXPECT_EQ(tokens[7].type, TokenType::OpShiftLeft);
	EXPECT_EQ(tokens[8].type, TokenType::OpShiftRight);
	EXPECT_EQ(tokens[9].type, TokenType::OpGreater);
	EXPECT_EQ(tokens[10].type, TokenType::OpGreaterEq);
	EXPECT_EQ(tokens[11].type, TokenType::OpGreater);
	EXPECT_EQ(tokens[12].type, TokenType::OpNotEq);
	EXPECT_EQ(tokens[13].type, TokenType::OpEq);
	EXPECT_EQ(tokens[14].type, TokenType::EndOfFile);
}

// Проверка генерации токена NewLine вне скобок и объединения пустых строк
TEST(LexerTest, NewlineOutsideBrackets)
{
	DiagnosticEngine engine;
	std::string_view input = "val\n\n\nvar";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::Keyword);
	EXPECT_EQ(tokens[1].type, TokenType::NewLine);
	EXPECT_EQ(tokens[2].type, TokenType::Keyword);
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка игнорирования переносов строк внутри круглых скобок
TEST(LexerTest, NewlineInsideParentheses)
{
	DiagnosticEngine engine;
	std::string_view input = "(\nval\n)";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::ParenLeft);
	EXPECT_EQ(tokens[1].type, TokenType::Keyword);
	EXPECT_EQ(tokens[2].type, TokenType::ParenRight);
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка игнорирования переносов строк внутри квадратных скобок
TEST(LexerTest, NewlineInsideBrackets)
{
	DiagnosticEngine engine;
	std::string_view input = "[\n123\n]";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::BracketLeft);
	EXPECT_EQ(tokens[1].type, TokenType::Integer);
	EXPECT_EQ(tokens[2].type, TokenType::BracketRight);
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка полного игнорирования комментариев и корректной обработки переноса строки после них
TEST(LexerTest, CommentIgnored)
{
	DiagnosticEngine engine;
	std::string_view input = "val # some comment text\nvar";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::Keyword);
	EXPECT_EQ(tokens[1].type, TokenType::NewLine);
	EXPECT_EQ(tokens[2].type, TokenType::Keyword);
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

// Проверка корректного подсчета позиции и строки для токенов
TEST(LexerTest, TokenPositionAndLine)
{
	DiagnosticEngine engine;
	std::string_view input = "val\n  var";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].pos, 0);
	EXPECT_EQ(tokens[0].line, 1);
	EXPECT_EQ(tokens[1].pos, 3);
	EXPECT_EQ(tokens[1].line, 1);
	EXPECT_EQ(tokens[2].pos, 6);
	EXPECT_EQ(tokens[2].line, 2);
	EXPECT_EQ(tokens[3].pos, 9);
	EXPECT_EQ(tokens[3].line, 2);
}

// Проверка продолжения работы лексера и генерации ошибки при встрече неизвестного символа
TEST(LexerTest, UnknownSymbolError)
{
	DiagnosticEngine engine;
	std::string_view input = "val $ var";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_EQ(tokens.size(), 4);
	EXPECT_EQ(tokens[0].type, TokenType::Keyword);
	EXPECT_EQ(tokens[1].type, TokenType::Error);
	EXPECT_EQ(tokens[2].type, TokenType::Keyword);
	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
	EXPECT_TRUE(engine.HasErrors());
}

// Проверка корректной токенизации валидного объявления функции на Weasel
TEST(LexerTest, FullProgram)
{
	DiagnosticEngine engine;
	std::string_view input = "voided <- main()\nval s number x := 10";

	auto tokens = Lexer::Tokenize(input, engine);

	ASSERT_FALSE(engine.HasErrors());
	ASSERT_EQ(tokens.size(), 13);
	EXPECT_EQ(tokens[0].type, TokenType::Keyword);
	EXPECT_EQ(tokens[0].value, "voided");
	EXPECT_EQ(tokens[1].type, TokenType::OpMove);
	EXPECT_EQ(tokens[2].type, TokenType::Identifier);
	EXPECT_EQ(tokens[2].value, "main");
	EXPECT_EQ(tokens[3].type, TokenType::ParenLeft);
	EXPECT_EQ(tokens[4].type, TokenType::ParenRight);
	EXPECT_EQ(tokens[5].type, TokenType::NewLine);
	EXPECT_EQ(tokens[6].type, TokenType::Keyword);
	EXPECT_EQ(tokens[6].value, "val");
	EXPECT_EQ(tokens[7].type, TokenType::Keyword);
	EXPECT_EQ(tokens[7].value, "s");
	EXPECT_EQ(tokens[8].type, TokenType::Keyword);
	EXPECT_EQ(tokens[8].value, "number");
	EXPECT_EQ(tokens[9].type, TokenType::Identifier);
	EXPECT_EQ(tokens[9].value, "x");
	EXPECT_EQ(tokens[10].type, TokenType::OpAssign);
	EXPECT_EQ(tokens[11].type, TokenType::Integer);
	EXPECT_EQ(tokens[11].value, "10");
	EXPECT_EQ(tokens[12].type, TokenType::EndOfFile);
}