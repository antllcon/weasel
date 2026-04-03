#include "grammar/GrammarTypes.h"
#include "grammar/grammarOptimizer/GrammarOptimizer.h"
#include "grammar/guideSetsCalculator/GuideSetsCalculator.h"
#include "grammar/llTableBuilder/Ll1TableBuilder.h"
#include "grammar/llTableBuilder/Ll1TablePrinter.h"
#include "grammar/parser/GrammarConsistencyChecker.h"
#include "grammar/parser/GrammarParser.h"
#include "grammar/printGrammar/PrintGrammar.h"
#include "grammar/tableBilder/Ll1Parser.h"
#include "grammar/tableBilder/ParseStepsPrinter.h"
#include "lexer/Lexer.h"
#include "lexer/token/Token.h"

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
	// 1. Прогоняем строку через твой лексер
	auto tokens = Lexer::Tokenize(input);
	std::vector<std::string> grammarTokens;

	// 2. Мапим типы токенов на терминалы грамматики
	for (const auto& token : tokens)
	{
		switch (token.type)
		{
		case TokenType::Identifier:
			grammarTokens.push_back("id"); // Было "P"
			break;
		case TokenType::Integer:
		case TokenType::Float:
			grammarTokens.push_back("num"); // Было "NUM"
			break;
		case TokenType::String:
			grammarTokens.push_back("str"); // Было "STR"
			break;
		case TokenType::Keyword:
			// Ключевые слова (true, false, and, orr, not) забираем как есть
			grammarTokens.push_back(std::string(token.value));
			break;
		case TokenType::EndOfFile:
			// Твой лексер съедает символ '#' как комментарий,
			// поэтому конец строки (EOF) мы вручную превращаем в маркер конца '#'
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
			// Можно игнорировать или кидать ошибку для неподдерживаемых токенов
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

		const auto result = GrammarOptimizer::FindBestLL1(rawRules, startSymbol);
		if (!result.isFound) {
			throw std::runtime_error("Грамматика не LL(1) даже после оптимизаций!");
		}

		const std::string newStartSymbol = result.startSymbol;

		std::cout << "Примененные оптимизации (с сохранением языка):" << std::endl;
		std::cout << "  - Добавлен стартовый маркер (Z -> S #)" << std::endl;
		PrintAppliedOptimizations(result.flags);
		PrintRules(result.rules, "Итоговая грамматика:");

		GuideSetsCalculator calculator(result.rules, newStartSymbol);
		Rules rulesWithGuides = calculator.Calculate();
		PrintRulesWithGuides(rulesWithGuides, "Направляющие множества грамматик:");

		Ll1TableBuilder tableBuilder(rulesWithGuides);
		const auto ll1Table = tableBuilder.Build();
		Ll1TablePrinter::Print(ll1Table);

		std::string inputLine = "a <= b <= c";
		std::vector<std::string> tokens = LexerToGrammarTokens(inputLine);

		Ll1Parser parser(ll1Table, newStartSymbol);
		auto steps = parser.Parse(tokens);

		ParseStepsPrinter::Print(steps, inputLine);
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}