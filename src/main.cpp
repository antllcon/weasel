#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/lalrTableBuilder/Lalr1Parser.h"
#include "grammar/lalrTableBuilder/Lalr1TableBuilder.h"
#include "grammar/lalrTableBuilder/Lalr1TablePrinter.h"
#include "grammar/lalrTableBuilder/LalrParseStepsPrinter.h"
#include "grammar/parser/GrammarConsistencyChecker.h"
#include "grammar/parser/GrammarParser.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "lexer/Lexer.h"
#include "lexer/Token.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

using namespace PrintGrammar;

namespace
{
std::vector<std::string> LexerToGrammarTokens(const std::string& input)
{
	auto tokens = Lexer::Tokenize(input);
	std::vector<std::string> grammarTokens;

	for (const auto& token : tokens)
	{
		switch (token.type)
		{
		case TokenType::Identifier:
			if (token.value == "a" || token.value == "b" || token.value == "c")
			{
				grammarTokens.push_back(std::string(token.value));
			}
			else
			{
				grammarTokens.push_back("id");
			}
			break;
		case TokenType::Integer:
		case TokenType::Float:
			grammarTokens.push_back("num");
			break;
		case TokenType::String:
			grammarTokens.push_back("str");
			break;
		case TokenType::Keyword:
			grammarTokens.push_back(std::string(token.value));
			break;
		case TokenType::EndOfFile:
			grammarTokens.push_back("#");
			break;
		case TokenType::OpPlus:
			grammarTokens.push_back("+");
			break;
		case TokenType::OpMinus:
			grammarTokens.push_back("-");
			break;
		case TokenType::OpMul:
			grammarTokens.push_back("*");
			break;
		case TokenType::OpDiv:
			grammarTokens.push_back("/");
			break;
		case TokenType::OpMod:
			grammarTokens.push_back("%");
			break;
		case TokenType::OpEq:
			grammarTokens.push_back("==");
			break;
		case TokenType::OpNotEq:
			grammarTokens.push_back("><");
			break;
		case TokenType::OpLess:
			grammarTokens.push_back("<");
			break;
		case TokenType::OpGreater:
			grammarTokens.push_back(">");
			break;
		case TokenType::OpLessEq:
			grammarTokens.push_back("<=");
			break;
		case TokenType::OpGreaterEq:
			grammarTokens.push_back(">=");
			break;
		case TokenType::ParenLeft:
			grammarTokens.push_back("(");
			break;
		case TokenType::ParenRight:
			grammarTokens.push_back(")");
			break;
		case TokenType::BracketLeft:
			grammarTokens.push_back("[");
			break;
		case TokenType::BracketRight:
			grammarTokens.push_back("]");
			break;
		case TokenType::Dot:
			grammarTokens.push_back(".");
			break;
		default:
			break;
		}
	}

	return grammarTokens;
}
} // namespace

int main()
{
	try
	{
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		const std::filesystem::path grammarFile = "data.txt";
		raw::Rules rawRules = GrammarParser::ParseFile(grammarFile);
		GrammarConsistencyChecker::Check(rawRules);

		const std::string startSymbol = rawRules.empty() ? "S" : rawRules.front().name;
		PrintRules(rawRules, "Исходная грамматика:");

		rawRules = GrammarOptimizer::OptimizeForLalr(std::move(rawRules), startSymbol);

		std::string newStartSymbol;
		rawRules = GrammarOptimizer::AugmentGrammarLalr(std::move(rawRules), startSymbol, newStartSymbol);

		PrintRules(rawRules, "Подготовленная грамматика для LALR(1):");

		Lalr1TableBuilder tableBuilder(rawRules, newStartSymbol);
		const auto lalrTable = tableBuilder.Build();

		Lalr1TablePrinter::Print(lalrTable);

		std::string inputLine = "";
		std::vector<std::string> tokens = LexerToGrammarTokens(inputLine);

		Lalr1Parser parser(lalrTable);
		auto steps = parser.Parse(tokens);

		LalrParseStepsPrinter::Print(steps, inputLine);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}