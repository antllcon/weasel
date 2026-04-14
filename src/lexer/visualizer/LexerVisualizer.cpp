#include "LexerVisualizer.h"
#include <chrono>
#include <iostream>
#include <thread>

namespace
{
constexpr std::string_view ColorReset = "\033[0m";
constexpr std::string_view ColorKeyword = "\033[35m";
constexpr std::string_view ColorString = "\033[32m";
constexpr std::string_view ColorNumber = "\033[36m";
constexpr std::string_view ColorOperator = "\033[33m";
constexpr std::string_view ColorError = "\033[31m";
constexpr std::string_view ColorDefault = "\033[37m";

std::string_view GetTokenColor(TokenType type)
{
	switch (type)
	{
	case TokenType::Keyword:
		return ColorKeyword;
	case TokenType::String:
		return ColorString;
	case TokenType::Integer:
	case TokenType::Float:
		return ColorNumber;
	case TokenType::OpAssign:
	case TokenType::OpMove:
	case TokenType::OpPlus:
	case TokenType::OpMinus:
	case TokenType::OpMul:
	case TokenType::OpDiv:
	case TokenType::OpEq:
	case TokenType::OpLess:
	case TokenType::OpGreater:
		return ColorOperator;
	case TokenType::Error:
		return ColorError;
	default:
		return ColorDefault;
	}
}

std::string TokenTypeToString(TokenType type)
{
	switch (type)
	{
	case TokenType::Keyword:
		return "Keyword";
	case TokenType::Identifier:
		return "Identifier";
	case TokenType::Integer:
		return "Integer";
	case TokenType::Float:
		return "Float";
	case TokenType::String:
		return "String";
	case TokenType::OpAssign:
		return "OpAssign";
	case TokenType::OpMove:
		return "OpMove";
	case TokenType::BraceLeft:
		return "BraceLeft";
	case TokenType::BraceRight:
		return "BraceRight";
	case TokenType::BracketLeft:
		return "BracketLeft";
	case TokenType::BracketRight:
		return "BracketRight";
	case TokenType::Error:
		return "Error";
	case TokenType::EndOfFile:
		return "EOF";
	default:
		return "Punctuation";
	}
}

void PrintDiagnostics(const DiagnosticEngine& engine)
{
	if (!engine.HasErrors())
	{
		return;
	}

	std::cout << "\n"
			  << ColorError
			  << "=== ОБНАРУЖЕНЫ ОШИБКИ (" << engine.GetInfo().size() << ") ==="
			  << ColorReset << std::endl;

	for (const auto& diag : engine.GetInfo())
	{
		std::cout << "[Строка " << diag.line << ", Позиция " << diag.pos << "] "
				  << diag.message << std::endl;
	}
}

void AnimateToken(const Token& token)
{
	std::cout << GetTokenColor(token.type)
			  << "[" << TokenTypeToString(token.type) << "] "
			  << "Лексема: '" << token.value << "' "
			  << "(Строка: " << token.line << ", Позиция: " << token.pos << ")"
			  << ColorReset << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(15));
}
} // namespace

void LexerVisualizer::Visualize(const std::vector<Token>& tokens, const DiagnosticEngine& engine)
{
	std::cout << "[Visualizer lexer]" << std::endl;

	for (const auto& token : tokens)
	{
		AnimateToken(token);
	}

	PrintDiagnostics(engine);

	std::cout << "[Visualizer lexer]" << std::endl;
}