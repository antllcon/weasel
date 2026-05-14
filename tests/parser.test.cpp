#include "src/grammar/lalr/LalrParser.h"
#include "src/grammar/context/LanguageContextBuilder.h"
#include "src/grammar/cst/CstNode.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace testing;

namespace
{
    CstInputToken Tok(std::string symbol, std::string value = "")
    {
        return CstInputToken{std::move(symbol), std::move(value), {}};
    }

    const CstNode* Find(const CstNode* node, const std::string& label)
    {
        if (!node)
        {
            return nullptr;
        }
        if (node->label == label)
        {
            return node;
        }
        for (const auto& child : node->children)
        {
            if (auto* found = Find(child.get(), label))
            {
                return found;
            }
        }
        return nullptr;
    }

	[[maybe_unused]] const CstNode* Navigate(const CstNode* node, std::initializer_list<size_t> path)
    {
        const CstNode* cur = node;
        for (size_t idx : path)
        {
            if (!cur || idx >= cur->children.size())
            {
                return nullptr;
            }
            cur = cur->children[idx].get();
        }
        return cur;
    }

    std::vector<std::string> LeafValues(const CstNode* node)
    {
        if (!node)
        {
            return {};
        }
        if (node->children.empty())
        {
            return {node->value};
        }

        std::vector<std::string> result;
        for (const auto& child : node->children)
        {
            for (auto& v : LeafValues(child.get()))
            {
                result.push_back(v);
            }
        }
        return result;
    }

    std::vector<CstInputToken> WrapInFunc(const std::vector<CstInputToken>& stmts)
    {
        std::vector<CstInputToken> res = {
            Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "f"),
            Tok("(", "("), Tok(")", ")"), Tok("{", "{")
        };
        res.insert(res.end(), stmts.begin(), stmts.end());
        res.push_back(Tok("}", "}"));
        res.push_back(Tok("#", ""));
        return res;
    }
}

class ParserTest : public Test
{
protected:
    static void SetUpTestSuite()
    {
        s_context = std::make_unique<LanguageContext>(
            LanguageContextBuilder::Build(TEST_GRAMMAR_FILE));
    }

    static void TearDownTestSuite()
    {
        s_context.reset();
    }

	static const LalrTable& Table()
	{
        return *s_context->lalrTable;
    }

	static std::unique_ptr<CstNode> Parse(const std::vector<CstInputToken>& tokens)
	{
        return LalrParser::ParseToTree(Table(), tokens);
    }

    static std::unique_ptr<LanguageContext> s_context;
};

std::unique_ptr<LanguageContext> ParserTest::s_context;

// Проверка выбрасывания исключения при пустом векторе токенов
TEST_F(ParserTest, EmptyTokensThrows)
{
    EXPECT_THROW(Parse({}), std::runtime_error);
}

// Проверка выбрасывания исключения при неожиданном токене
TEST_F(ParserTest, UnexpectedTokenThrows)
{
    EXPECT_THROW(Parse({Tok("num", "144"), Tok("#", "")}), std::runtime_error);
}

// Проверка выбрасывания исключения при незавершенном выражении
TEST_F(ParserTest, IncompleteExprThrows)
{
    EXPECT_THROW(Parse({
        Tok("val", "val"), Tok("number", "number"), Tok("id", "x"),
        Tok(":=", ":="), Tok("#", "")
    }), std::runtime_error);
}

// Проверка парсинга пустой программы
TEST_F(ParserTest, EmptyProgram)
{
    auto root = Parse({Tok("#", "")});

    ASSERT_NE(root, nullptr);
    auto declList = Find(root.get(), "DeclList");
    ASSERT_NE(declList, nullptr);
    EXPECT_TRUE(declList->children.empty());
}

// Проверка корректного корневого узла для пустой грамматики
TEST_F(ParserTest, RootLabelIsAugmentedStart)
{
    auto root = Parse({Tok("#", "")});

    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->label, "Z");
    ASSERT_EQ(root->children.size(), 1);
    EXPECT_EQ(root->children[0]->label, "Program");
}

// Проверка парсинга функции без параметров
TEST_F(ParserTest, FuncDeclNoParams)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "main"),
        Tok("(", "("), Tok(")", ")"), Tok("{", "{"), Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto funcDecl = Find(root.get(), "FuncDecl");
    ASSERT_NE(funcDecl, nullptr);
    ASSERT_EQ(funcDecl->children.size(), 8);
    EXPECT_EQ(funcDecl->children[1]->value, "<-");
    EXPECT_EQ(funcDecl->children[2]->value, "main");

    auto type = Find(root.get(), "Type");
    ASSERT_NE(type, nullptr);
    ASSERT_FALSE(type->children.empty());
    EXPECT_EQ(type->children[0]->label, "BaseType");
}

// Проверка парсинга функции с параметрами
TEST_F(ParserTest, FuncDeclWithParams)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "add"), Tok("(", "("),
        Tok("s", "s"), Tok("number", "number"), Tok("id", "a"), Tok(",", ","),
        Tok("s", "s"), Tok("number", "number"), Tok("id", "b"),
        Tok(")", ")"), Tok("{", "{"), Tok("return", "return"), Tok("nl", "\n"),
        Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto paramList = Find(root.get(), "ParamList");
    ASSERT_NE(paramList, nullptr);
    EXPECT_EQ(paramList->children.size(), 3);
}

// Проверка парсинга функции с пользовательским типом возвращаемого значения
TEST_F(ParserTest, FuncDeclReturnTypeId)
{
    auto root = Parse({
        Tok("id", "MyStruct"), Tok("<-", "<-"), Tok("id", "create"),
        Tok("(", "("), Tok(")", ")"), Tok("{", "{"), Tok("return", "return"),
        Tok("nl", "\n"), Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto type = Find(root.get(), "Type");
    ASSERT_NE(type, nullptr);
    ASSERT_FALSE(type->children.empty());
    EXPECT_EQ(type->children[0]->label, "id");
    EXPECT_EQ(type->children[0]->value, "MyStruct");
}

// Проверка парсинга функции со знаковым типом
TEST_F(ParserTest, FuncDeclSignedType)
{
    auto root = Parse({
        Tok("s", "s"), Tok("number", "number"), Tok("<-", "<-"), Tok("id", "foo"),
        Tok("(", "("), Tok(")", ")"), Tok("{", "{"), Tok("return", "return"),
        Tok("nl", "\n"), Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto sign = Find(root.get(), "Sign");
    ASSERT_NE(sign, nullptr);
    ASSERT_FALSE(sign->children.empty());
    EXPECT_EQ(sign->children[0]->value, "s");

    auto baseType = Find(root.get(), "BaseType");
    ASSERT_NE(baseType, nullptr);
    ASSERT_FALSE(baseType->children.empty());
    EXPECT_EQ(baseType->children[0]->value, "number");
}

// Проверка объявления переменной с инициализацией
TEST_F(ParserTest, VarDeclWithAssign)
{
    auto root = Parse(WrapInFunc({
        Tok("val", "val"), Tok("s", "s"), Tok("number", "number"),
        Tok("id", "x"), Tok(":=", ":="), Tok("num", "10"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto varDecl = Find(root.get(), "VarDecl");
    ASSERT_NE(varDecl, nullptr);
    ASSERT_EQ(varDecl->children.size(), 5);

    EXPECT_EQ(varDecl->children[0]->children[0]->value, "val");
    EXPECT_EQ(varDecl->children[2]->value, "x");
    EXPECT_EQ(varDecl->children[3]->children[0]->value, ":=");

    auto expr = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(expr, nullptr);
    EXPECT_EQ(expr->children[0]->value, "10");
}

// Проверка объявления переменной без инициализации
TEST_F(ParserTest, VarDeclNoAssign)
{
    auto root = Parse(WrapInFunc({
        Tok("var", "var"), Tok("number", "number"), Tok("id", "y"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto varDecl = Find(root.get(), "VarDecl");
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->children.size(), 3);
}

// Проверка объявления переменной с move-инициализацией
TEST_F(ParserTest, VarDeclMoveAssign)
{
    auto root = Parse(WrapInFunc({
        Tok("val", "val"), Tok("number", "number"), Tok("id", "z"),
        Tok("<-", "<-"), Tok("num", "0"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto assignOp = Find(root.get(), "AssignOp");
    ASSERT_NE(assignOp, nullptr);
    EXPECT_EQ(assignOp->children[0]->value, "<-");
}

// Проверка объявления переменной с модификатором def
TEST_F(ParserTest, VarDeclDefModifier)
{
    auto root = Parse(WrapInFunc({
        Tok("def", "def"), Tok("number", "number"), Tok("id", "MAX_VALUE"),
        Tok(":=", ":="), Tok("num", "100"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto modifier = Find(root.get(), "Modifier");
    ASSERT_NE(modifier, nullptr);
    EXPECT_EQ(modifier->children[0]->value, "def");
}

// Проверка простого оператора присваивания
TEST_F(ParserTest, SimpleAssign)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "x"), Tok(":=", ":="), Tok("num", "5"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto assignStmt = Find(root.get(), "AssignStmt");
    ASSERT_NE(assignStmt, nullptr);
    ASSERT_EQ(assignStmt->children.size(), 3);
    EXPECT_EQ(assignStmt->children[1]->children[0]->value, ":=");
}

// Проверка парсинга одиночного числа
TEST_F(ParserTest, SingleNumber)
{
    auto root = Parse(WrapInFunc({Tok("num", "67"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->label, "num");
    EXPECT_EQ(primary->children[0]->value, "67");
}

// Проверка левоассоциативности сложения
TEST_F(ParserTest, AdditionLeftAssoc)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("+", "+"), Tok("id", "b"), Tok("+", "+"),
        Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto addExpr = Find(root.get(), "AddExpr");
    ASSERT_NE(addExpr, nullptr);
    ASSERT_EQ(addExpr->children.size(), 3);
    EXPECT_EQ(addExpr->children[0]->label, "AddExpr");
}

// Проверка приоритета умножения над сложением
TEST_F(ParserTest, MultiplicationHigherPrecedence)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("+", "+"), Tok("id", "b"), Tok("*", "*"),
        Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto addExpr = Find(root.get(), "AddExpr");
    ASSERT_NE(addExpr, nullptr);
    ASSERT_EQ(addExpr->children.size(), 3);
    EXPECT_EQ(addExpr->children[2]->label, "MulExpr");
}

// Проверка приоритета умножения перед сложением
TEST_F(ParserTest, MultiplicationLeftBeforeAddition)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("*", "*"), Tok("id", "b"), Tok("+", "+"),
        Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto addExpr = Find(root.get(), "AddExpr");
    ASSERT_NE(addExpr, nullptr);
    ASSERT_EQ(addExpr->children.size(), 3);
    EXPECT_EQ(addExpr->children[0]->label, "AddExpr");

    auto leftMul = Find(addExpr->children[0].get(), "MulExpr");
    ASSERT_NE(leftMul, nullptr);
}

// Проверка логического отрицания
TEST_F(ParserTest, UnaryNot)
{
    auto root = Parse(WrapInFunc({Tok("not", "not"), Tok("id", "x"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto unaryExpr = Find(root.get(), "UnaryExpr");
    ASSERT_NE(unaryExpr, nullptr);
    ASSERT_EQ(unaryExpr->children.size(), 2);
    EXPECT_EQ(unaryExpr->children[0]->value, "not");
}

// Проверка операции взятия адреса
TEST_F(ParserTest, UnaryAddress)
{
    auto root = Parse(WrapInFunc({Tok("&", "&"), Tok("id", "x"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto unaryExpr = Find(root.get(), "UnaryExpr");
    ASSERT_NE(unaryExpr, nullptr);
    ASSERT_EQ(unaryExpr->children.size(), 2);
    EXPECT_EQ(unaryExpr->children[0]->value, "&");
}

// Проверка операции разыменования указателя
TEST_F(ParserTest, UnaryPointerDeref)
{
    auto root = Parse(WrapInFunc({Tok("*", "*"), Tok("id", "x"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto unaryExpr = Find(root.get(), "UnaryExpr");
    ASSERT_NE(unaryExpr, nullptr);
    ASSERT_EQ(unaryExpr->children.size(), 2);
    EXPECT_EQ(unaryExpr->children[0]->value, "*");
}

// Проверка приоритета логического И над логическим ИЛИ
TEST_F(ParserTest, LogicalAndOrPrecedence)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("orr", "orr"), Tok("id", "b"),
        Tok("and", "and"), Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto orExpr = Find(root.get(), "LogicOrExpr");
    ASSERT_NE(orExpr, nullptr);
    ASSERT_EQ(orExpr->children.size(), 3);
    EXPECT_EQ(orExpr->children[2]->label, "LogicAndExpr");
}

// Проверка приоритета равенства и отношений
TEST_F(ParserTest, EqRelPrecedence)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("==", "=="), Tok("id", "b"),
        Tok("<", "<"), Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto eqExpr = Find(root.get(), "EqExpr");
    ASSERT_NE(eqExpr, nullptr);
    ASSERT_EQ(eqExpr->children.size(), 3);
    EXPECT_EQ(eqExpr->children[2]->label, "RelExpr");
}

// Проверка левоассоциативности побитовых операций
TEST_F(ParserTest, BitwiseOperators)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("bnd", "bnd"), Tok("id", "b"),
        Tok("bor", "bor"), Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto bitwiseExpr = Find(root.get(), "BitwiseExpr");
    ASSERT_NE(bitwiseExpr, nullptr);
    ASSERT_EQ(bitwiseExpr->children.size(), 3);
    EXPECT_EQ(bitwiseExpr->children[0]->label, "BitwiseExpr");
}

// Проверка левоассоциативности сдвигов
TEST_F(ParserTest, ShiftOperators)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("<<", "<<"), Tok("num", "1"),
        Tok(">>", ">>"), Tok("num", "2"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto shiftExpr = Find(root.get(), "ShiftExpr");
    ASSERT_NE(shiftExpr, nullptr);
    ASSERT_EQ(shiftExpr->children.size(), 3);
    EXPECT_EQ(shiftExpr->children[0]->label, "ShiftExpr");
}

// Проверка скобочных выражений
TEST_F(ParserTest, ParenthesizedExpr)
{
    auto root = Parse(WrapInFunc({
        Tok("(", "("), Tok("id", "a"), Tok("+", "+"), Tok("id", "b"),
        Tok(")", ")"), Tok("*", "*"), Tok("id", "c"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    ASSERT_EQ(primary->children.size(), 3);
    EXPECT_EQ(primary->children[0]->value, "(");
    EXPECT_EQ(primary->children[1]->label, "Expr");
}

// Проверка строковых литералов
TEST_F(ParserTest, StringLiteral)
{
    auto root = Parse(WrapInFunc({Tok("str", "\"hello\""), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->label, "str");
    EXPECT_EQ(primary->children[0]->value, "\"hello\"");
}

// Проверка булевых литералов
TEST_F(ParserTest, TrueFalseLiterals)
{
    auto root = Parse(WrapInFunc({Tok("true", "true"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->value, "true");
}

// Проверка вызова функции без аргументов
TEST_F(ParserTest, FuncCallNoArgs)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "f"), Tok("(", "("), Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children.size(), 3);
    EXPECT_EQ(primary->children[0]->value, "f");
}

// Проверка вызова функции с одним аргументом
TEST_F(ParserTest, FuncCallOneArg)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "f"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    ASSERT_EQ(primary->children.size(), 4);
    EXPECT_EQ(primary->children[2]->label, "ArgList");
}

// Проверка вызова функции с несколькими аргументами
TEST_F(ParserTest, FuncCallMultipleArgs)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "f"), Tok("(", "("), Tok("id", "a"), Tok(",", ","),
        Tok("id", "b"), Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto argList = Find(root.get(), "ArgList");
    ASSERT_NE(argList, nullptr);
    EXPECT_EQ(argList->children.size(), 3);
}

// Проверка вызова метода без аргументов
TEST_F(ParserTest, MethodCallNoArgs)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "x"), Tok(".", "."), Tok("id", "method"),
        Tok("(", "("), Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto postfix = Find(root.get(), "PostfixExpr");
    ASSERT_NE(postfix, nullptr);
    EXPECT_EQ(postfix->children.size(), 5);
}

// Проверка вызова метода с аргументами
TEST_F(ParserTest, MethodCallWithArgs)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "x"), Tok(".", "."), Tok("id", "method"),
        Tok("(", "("), Tok("id", "a"), Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto postfix = Find(root.get(), "PostfixExpr");
    ASSERT_NE(postfix, nullptr);
    EXPECT_EQ(postfix->children.size(), 6);
}

// Проверка доступа по индексу массива
TEST_F(ParserTest, IndexAccess)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "x"), Tok("[", "["), Tok("id", "i"), Tok("]", "]"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto postfix = Find(root.get(), "PostfixExpr");
    ASSERT_NE(postfix, nullptr);
    EXPECT_EQ(postfix->children.size(), 4);
    EXPECT_EQ(postfix->children[1]->value, "[");
}

// Проверка цепочки постфиксных операций
TEST_F(ParserTest, ChainedPostfix)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "x"), Tok(".", "."), Tok("id", "field"),
        Tok("[", "["), Tok("num", "0"), Tok("]", "]"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto topPostfix = Find(root.get(), "PostfixExpr");
    ASSERT_NE(topPostfix, nullptr);
    EXPECT_EQ(topPostfix->children.size(), 4);
    EXPECT_EQ(topPostfix->children[0]->label, "PostfixExpr");
}

// Проверка литерала массива
TEST_F(ParserTest, ArrayLiteral)
{
    auto root = Parse(WrapInFunc({
        Tok("[", "["), Tok("id", "a"), Tok(",", ","), Tok("id", "b"),
        Tok("]", "]"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children.size(), 3);
    EXPECT_EQ(primary->children[0]->value, "[");
}

// Проверка условного оператора без ветки else
TEST_F(ParserTest, WhenNoElse)
{
    auto root = Parse(WrapInFunc({
        Tok("when", "when"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"),
        Tok("{", "{"), Tok("return", "return"), Tok("nl", "\n"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto ifStmt = Find(root.get(), "IfStmt");
    ASSERT_NE(ifStmt, nullptr);
    ASSERT_EQ(ifStmt->children.size(), 8);

    auto elseOpt = Find(ifStmt, "ElseOpt");
    ASSERT_NE(elseOpt, nullptr);
    EXPECT_TRUE(elseOpt->children.empty());
}

// Проверка условного оператора с веткой else
TEST_F(ParserTest, WhenWithElse)
{
    auto root = Parse(WrapInFunc({
        Tok("when", "when"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"),
        Tok("{", "{"), Tok("}", "}"), Tok("else", "else"), Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto elseOpt = Find(root.get(), "ElseOpt");
    ASSERT_NE(elseOpt, nullptr);
    EXPECT_EQ(elseOpt->children.size(), 4);
}

// Проверка цепочки условий
TEST_F(ParserTest, WhenWithElseWhen)
{
    auto root = Parse(WrapInFunc({
        Tok("when", "when"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"), Tok("{", "{"), Tok("}", "}"),
        Tok("else", "else"), Tok("(", "("), Tok("id", "y"), Tok(")", ")"), Tok("{", "{"), Tok("}", "}"),
        Tok("else", "else"), Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto firstElseOpt = Find(root.get(), "ElseOpt");
    ASSERT_NE(firstElseOpt, nullptr);
    EXPECT_EQ(firstElseOpt->children.size(), 8);
}

// Проверка сохранения выражения в условии when
TEST_F(ParserTest, WhenConditionPreservesExpr)
{
    auto root = Parse(WrapInFunc({
        Tok("when", "when"), Tok("(", "("), Tok("id", "a"), Tok("<", "<"),
        Tok("id", "b"), Tok(")", ")"), Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto relExpr = Find(root.get(), "RelExpr");
    ASSERT_NE(relExpr, nullptr);
    EXPECT_EQ(relExpr->children.size(), 3);
}

// Проверка цикла while
TEST_F(ParserTest, WhileLoop)
{
    auto root = Parse(WrapInFunc({
        Tok("run", "run"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"),
        Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto whileStmt = Find(root.get(), "WhileStmt");
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_EQ(whileStmt->children.size(), 7);
}

// Проверка цикла do-while
TEST_F(ParserTest, DoWhileLoop)
{
    auto root = Parse(WrapInFunc({
        Tok("{", "{"), Tok("}", "}"), Tok("run", "run"), Tok("(", "("),
        Tok("id", "x"), Tok(")", ")")
    }));

    ASSERT_NE(root, nullptr);
    auto doWhileStmt = Find(root.get(), "DoWhileStmt");
    ASSERT_NE(doWhileStmt, nullptr);
    EXPECT_EQ(doWhileStmt->children.size(), 7);
}

// Проверка одномерного цикла for
TEST_F(ParserTest, ForLoop1D)
{
    auto root = Parse(WrapInFunc({
        Tok("rep", "rep"), Tok("(", "("), Tok("id", "i"), Tok("in", "in"),
        Tok("num", "0"), Tok("..", ".."), Tok("num", "10"), Tok(")", ")"),
        Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto forStmt = Find(root.get(), "ForStmt");
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->children.size(), 11);
}

// Проверка двумерного цикла for
TEST_F(ParserTest, ForLoop2D)
{
    auto root = Parse(WrapInFunc({
        Tok("rep", "rep"), Tok("(", "("), Tok("id", "x"), Tok(",", ","), Tok("id", "y"),
        Tok("in", "in"), Tok("num", "0"), Tok("..", ".."), Tok("num", "10"), Tok(",", ","),
        Tok("num", "0"), Tok("..", ".."), Tok("num", "20"), Tok(")", ")"),
        Tok("{", "{"), Tok("}", "}")
    }));

    ASSERT_NE(root, nullptr);
    auto forStmt = Find(root.get(), "ForStmt");
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->children.size(), 17);
}

// Проверка возврата с выражением
TEST_F(ParserTest, ReturnWithExpr)
{
    auto root = Parse(WrapInFunc({Tok("return", "return"), Tok("num", "67"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto retStmt = Find(root.get(), "ReturnStmt");
    ASSERT_NE(retStmt, nullptr);
    EXPECT_EQ(retStmt->children.size(), 2);
}

// Проверка возврата без выражения
TEST_F(ParserTest, ReturnVoid)
{
    auto root = Parse(WrapInFunc({Tok("return", "return"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto retStmt = Find(root.get(), "ReturnStmt");
    ASSERT_NE(retStmt, nullptr);
    EXPECT_EQ(retStmt->children.size(), 1);
}

// Проверка левоассоциативности списка операторов
TEST_F(ParserTest, StmtListLeftRecursion)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok(":=", ":="), Tok("num", "1"), Tok("nl", "\n"),
        Tok("id", "b"), Tok(":=", ":="), Tok("num", "2"), Tok("nl", "\n"),
        Tok("id", "c"), Tok(":=", ":="), Tok("num", "3"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    auto stmtList = Find(root.get(), "StmtList");
    ASSERT_NE(stmtList, nullptr);
    EXPECT_EQ(stmtList->children.size(), 2);
}

// Проверка левоассоциативности списка объявлений
TEST_F(ParserTest, DeclListLeftRecursion)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "f"), Tok("(", "("), Tok(")", ")"), Tok("{", "{"), Tok("}", "}"),
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "g"), Tok("(", "("), Tok(")", ")"), Tok("{", "{"), Tok("}", "}"),
        Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto declList = Find(root.get(), "DeclList");
    ASSERT_NE(declList, nullptr);
    EXPECT_EQ(declList->children.size(), 2);
}

// Проверка объявления структуры
TEST_F(ParserTest, StructDecl)
{
    auto root = Parse({
        Tok("struct", "struct"), Tok("id", "Point"), Tok("[", "["),
        Tok("number", "number"), Tok("id", "x"), Tok("number", "number"), Tok("id", "y"),
        Tok("]", "]"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto structDecl = Find(root.get(), "StructDecl");
    ASSERT_NE(structDecl, nullptr);
    auto fieldList = Find(structDecl, "FieldList");
    ASSERT_NE(fieldList, nullptr);
}

// Проверка объявления объединения
TEST_F(ParserTest, UnionDecl)
{
    auto root = Parse({
        Tok("unions", "unions"), Tok("id", "Data"), Tok("[", "["),
        Tok("number", "number"), Tok("id", "as_int"), Tok("single", "single"), Tok("id", "as_flt"),
        Tok("]", "]"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto unionDecl = Find(root.get(), "UnionDecl");
    ASSERT_NE(unionDecl, nullptr);
}

// Проверка объявления перечисления
TEST_F(ParserTest, EnumDecl)
{
    auto root = Parse({
        Tok("enumer", "enumer"), Tok("id", "Color"), Tok("[", "["),
        Tok("id", "RED"), Tok(",", ","), Tok("id", "GREEN"), Tok(",", ","), Tok("id", "BLUE"),
        Tok("]", "]"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    auto enumDecl = Find(root.get(), "EnumDecl");
    ASSERT_NE(enumDecl, nullptr);
    auto enumIdList = Find(enumDecl, "EnumIdList");
    ASSERT_NE(enumIdList, nullptr);
}

// Проверка корректного сохранения всех листовых значений дерева
TEST_F(ParserTest, LeafValuesPreserved)
{
    std::vector<CstInputToken> tokens = {
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "f"), Tok("(", "("), Tok(")", ")"),
        Tok("{", "{"), Tok("}", "}"), Tok("#", "")
    };
    auto root = Parse(tokens);

    ASSERT_NE(root, nullptr);
    auto leaves = LeafValues(root.get());

    std::vector<std::string> real;
    for (const auto& v : leaves)
    {
        if (!v.empty())
        {
            real.push_back(v);
        }
    }

    const std::vector<std::string> expected = { "voided", "<-", "f", "(", ")", "{", "}" };
    ASSERT_EQ(real.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(real[i], expected[i]);
    }
}

// Проверка сохранения значений идентификаторов
TEST_F(ParserTest, IdentifierValuePreserved)
{
    auto root = Parse(WrapInFunc({Tok("id", "myVar"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->value, "myVar");
}

// Проверка сохранения значений чисел
TEST_F(ParserTest, NumberValuePreserved)
{
    auto root = Parse(WrapInFunc({Tok("num", "3.14"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->value, "3.14");
}

// Проверка сохранения значений строк
TEST_F(ParserTest, StringValuePreserved)
{
    auto root = Parse(WrapInFunc({Tok("str", "\"hello\""), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    auto primary = Find(root.get(), "PrimaryExpr");
    ASSERT_NE(primary, nullptr);
    EXPECT_EQ(primary->children[0]->value, "\"hello\"");
}

// Проверка сохранения символа переноса строки
TEST_F(ParserTest, NewlineValuePreserved)
{
    auto root = Parse(WrapInFunc({Tok("return", "return"), Tok("nl", "\n")}));

    ASSERT_NE(root, nullptr);
    const CstNode* nlNode = Find(root.get(), "nl");
    ASSERT_NE(nlNode, nullptr);
    EXPECT_EQ(nlNode->value, "\n");
}

// Проверка минимальной валидной функции
TEST_F(ParserTest, MinimalFunction)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "main"), Tok("(", "("), Tok(")", ")"),
        Tok("{", "{"), Tok("return", "return"), Tok("nl", "\n"), Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    EXPECT_NE(Find(root.get(), "FuncDecl"), nullptr);
}

// Проверка функции с переменной и возвратом
TEST_F(ParserTest, FunctionWithVarAndReturn)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "main"), Tok("(", "("), Tok(")", ")"),
        Tok("{", "{"),
        Tok("val", "val"), Tok("s", "s"), Tok("number", "number"), Tok("id", "x"), Tok(":=", ":="), Tok("num", "10"), Tok("nl", "\n"),
        Tok("return", "return"), Tok("nl", "\n"),
        Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    EXPECT_NE(Find(root.get(), "VarDecl"), nullptr);
    EXPECT_NE(Find(root.get(), "ReturnStmt"), nullptr);
}

// Проверка функции с условием
TEST_F(ParserTest, FunctionWithConditional)
{
    auto root = Parse({
        Tok("voided", "voided"), Tok("<-", "<-"), Tok("id", "main"), Tok("(", "("), Tok(")", ")"),
        Tok("{", "{"),
        Tok("val", "val"), Tok("number", "number"), Tok("id", "a"), Tok(":=", ":="), Tok("num", "5"), Tok("nl", "\n"),
        Tok("when", "when"), Tok("(", "("), Tok("id", "a"), Tok(")", ")"),
        Tok("{", "{"), Tok("return", "return"), Tok("nl", "\n"), Tok("}", "}"),
        Tok("}", "}"), Tok("#", "")
    });

    ASSERT_NE(root, nullptr);
    EXPECT_NE(Find(root.get(), "IfStmt"), nullptr);
}

// Проверка вложенных вызовов функций
TEST_F(ParserTest, NestedFunctionCalls)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "f"), Tok("(", "("),
        Tok("id", "g"), Tok("(", "("), Tok("id", "x"), Tok(")", ")"), Tok(",", ","),
        Tok("id", "h"), Tok("(", "("), Tok("id", "y"), Tok(",", ","), Tok("id", "z"), Tok(")", ")"),
        Tok(")", ")"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    EXPECT_NE(Find(root.get(), "ArgList"), nullptr);
}

// Проверка сложного арифметического выражения со всеми уровнями
TEST_F(ParserTest, ArithmeticWithAllLevels)
{
    auto root = Parse(WrapInFunc({
        Tok("id", "a"), Tok("+", "+"), Tok("id", "b"), Tok("*", "*"), Tok("id", "c"),
        Tok("-", "-"), Tok("id", "d"), Tok("/", "/"), Tok("id", "e"), Tok("nl", "\n")
    }));

    ASSERT_NE(root, nullptr);
    EXPECT_NE(Find(root.get(), "AddExpr"), nullptr);
    EXPECT_NE(Find(root.get(), "MulExpr"), nullptr);
}