#include "LexerVisualizer.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/utils/logger/Logger.h"
#include <iomanip>
#include <sstream>

namespace
{
constexpr size_t TokensPerRow = 4;
constexpr size_t ValueWidth = 10;
constexpr size_t TypeWidth = 11;

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
	case TokenType::NewLine:
		return "NewLine";
	case TokenType::OpAssign:
		return "OpAssign";
	case TokenType::OpMove:
		return "OpMove";
	case TokenType::OpEq:
		return "OpEq";
	case TokenType::OpNotEq:
		return "OpNotEq";
	case TokenType::OpLess:
		return "OpLess";
	case TokenType::OpGreater:
		return "OpGreater";
	case TokenType::OpLessEq:
		return "OpLessEq";
	case TokenType::OpGreaterEq:
		return "OpGreaterEq";
	case TokenType::OpPlus:
		return "OpPlus";
	case TokenType::OpMinus:
		return "OpMinus";
	case TokenType::OpMul:
		return "OpMul";
	case TokenType::OpDiv:
		return "OpDiv";
	case TokenType::OpMod:
		return "OpMod";
	case TokenType::OpShiftLeft:
		return "ShiftLeft";
	case TokenType::OpShiftRight:
		return "ShiftRight";
	case TokenType::OpBitAnd:
		return "BitAnd";
	case TokenType::OpRange:
		return "Range";
	case TokenType::BraceLeft:
		return "BraceLeft";
	case TokenType::BraceRight:
		return "BraceRight";
	case TokenType::BracketLeft:
		return "BracketLeft";
	case TokenType::BracketRight:
		return "BracketRight";
	case TokenType::ParenLeft:
		return "ParenLeft";
	case TokenType::ParenRight:
		return "ParenRight";
	case TokenType::Comma:
		return "Comma";
	case TokenType::Dot:
		return "Dot";
	case TokenType::Colon:
		return "Colon";
	case TokenType::At:
		return "At";
	case TokenType::EndOfFile:
		return "EOF";
	case TokenType::Error:
		return "Error";
	default:
		return "Unknown";
	}
}

std::string TokenDisplayValue(const Token& token)
{
	if (token.type == TokenType::NewLine) return "\\n";
	if (token.type == TokenType::EndOfFile) return "EOF";
	return std::string(token.value);
}

std::string FormatCell(const Token& token)
{
	std::ostringstream cell;
	cell << "[ "
		 << std::left << std::setw(ValueWidth) << TokenDisplayValue(token)
		 << " "
		 << std::left << std::setw(TypeWidth) << TokenTypeToString(token.type)
		 << "]";
	return cell.str();
}

std::string FormatRow(const std::vector<Token>& tokens, size_t from, size_t count)
{
	std::ostringstream row;
	for (size_t i = 0; i < count; ++i)
	{
		if (i > 0)
		{
			row << " ";
		}
		row << FormatCell(tokens[from + i]);
	}
	return row.str();
}

void PrintTokensByRow(const std::vector<Token>& tokens)
{
	for (size_t i = 0; i < tokens.size(); i += TokensPerRow)
	{
		const size_t count = std::min(TokensPerRow, tokens.size() - i);
		Logger::Log("\t\t" + FormatRow(tokens, i, count));
	}
}

void PrintTokensByLine(const std::vector<Token>& tokens)
{
	size_t currentIndex = 0;
	while (currentIndex < tokens.size())
	{
		size_t count = 0;
		const size_t currentLine = tokens[currentIndex].line;

		while (currentIndex + count < tokens.size() && tokens[currentIndex + count].line == currentLine)
		{
			++count;
		}

		Logger::Log("\t\t" + FormatRow(tokens, currentIndex, count));
		currentIndex += count;
	}
}
} // namespace

void LexerVisualizer::Visualize(const std::vector<Token>& tokens, const DiagnosticEngine& engine, bool splitBySourceLines)
{
	Logger::Log("[Lexer]\t\tПоток токенов (" + std::to_string(tokens.size()) + "):");

	if (splitBySourceLines)
	{
		PrintTokensByLine(tokens);
	}
	else
	{
		PrintTokensByRow(tokens);
	}

	for (const auto& diag : engine.GetDiagnostics())
	{
		Logger::Log(DiagnosticEngine::FormatMessage(diag));
	}
}