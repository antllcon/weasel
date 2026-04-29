#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/GrammarTypes.h"
#include "src/grammar/context/LanguageContextBuilder.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrParser.h"
#include "src/lexer/Lexer.h"
#include "src/lexer/Token.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{
LanguageContext BuildContext()
{
	return LanguageContextBuilder::Build(TEST_GRAMMAR_FILE, nullptr);
}

std::string MapTokenToSymbol(const Token& token)
{
	switch (token.type)
	{
	case TokenType::Identifier:
		return "id";
	case TokenType::Integer:
	case TokenType::Float:
		return "num";
	case TokenType::String:
		return "str";
	case TokenType::EndOfFile:
		return END_SYMBOL;
	default:
		return std::string(token.value);
	}
}

std::vector<CstInputToken> TokenizeToCst(const std::string& source)
{
	DiagnosticEngine engine;
	const auto tokens = Lexer::Tokenize(source, engine);

	std::vector<CstInputToken> cstTokens;
	cstTokens.reserve(tokens.size());

	for (const auto& token : tokens)
	{
		cstTokens.push_back(CstInputToken{
			.symbol = MapTokenToSymbol(token),
			.value = token.value,
			.location = SourceLocation{token.line, token.pos}});
	}

	return cstTokens;
}

const CstNode* FindNodeByLabel(const CstNode& root, const std::string& label)
{
	if (root.label == label)
	{
		return &root;
	}

	for (const auto& child : root.children)
	{
		const auto* found = FindNodeByLabel(*child, label);
		if (found)
		{
			return found;
		}
	}

	return nullptr;
}
} // namespace

class CstTest : public Test
{
protected:
	static void SetUpTestSuite()
	{
		m_context = BuildContext();
	}

	std::unique_ptr<CstNode> Parse(const std::string& source) const
	{
		const auto cstTokens = TokenizeToCst(source);
		LalrParser parser(*m_context.lalrTable);
		return parser.ParseToTree(cstTokens);
	}

	static LanguageContext m_context;
};

LanguageContext CstTest::m_context;

// Проверка корня дерева при пустой программе
TEST_F(CstTest, EmptyProgramHasRoot)
{
	const auto root = Parse("");

	ASSERT_NE(root, nullptr);
	EXPECT_EQ(root->label, "Z");
}

// Проверка наличия узла Program внутри дерева
TEST_F(CstTest, EmptyProgramContainsProgramNode)
{
	const auto root = Parse("");

	const auto* programNode = FindNodeByLabel(*root, "Program");
	EXPECT_NE(programNode, nullptr);
}

// Проверка наличия узла VarDecl при объявлении переменной
TEST_F(CstTest, VarDeclCreatesVarDeclNode)
{
	const auto root = Parse("var number x := 42");

	ASSERT_NE(root, nullptr);
	const auto* varDecl = FindNodeByLabel(*root, "VarDecl");
	EXPECT_NE(varDecl, nullptr);
}

// Проверка листового узла с правильным значением
TEST_F(CstTest, VarDeclLeafHasCorrectValue)
{
	const auto root = Parse("var number x := 42");

	ASSERT_NE(root, nullptr);
	const auto* numNode = FindNodeByLabel(*root, "num");
	ASSERT_NE(numNode, nullptr);
	EXPECT_EQ(numNode->value, "42");
}

// Проверка построения дерева для объявления функции
TEST_F(CstTest, FuncDeclCreatesFuncDeclNode)
{
	const auto root = Parse("voided <- main() { }");

	ASSERT_NE(root, nullptr);
	const auto* funcDecl = FindNodeByLabel(*root, "FuncDecl");
	EXPECT_NE(funcDecl, nullptr);
}
