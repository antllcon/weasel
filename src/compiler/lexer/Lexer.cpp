#include "Lexer.h"

#include "src/compiler/core/LanguageTokens.h"
#include "src/diagnostics/CompilationException.h"
#include "visualizer/LexerVisualizer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace
{
constexpr size_t InitialPosition = 0;
constexpr size_t InitialLine = 1;
constexpr size_t AverageTokenLength = 5;

struct LexerState
{
	std::string_view source;
	[[maybe_unused]] DiagnosticEngine& engine;
	size_t pos = InitialPosition;
	size_t line = InitialLine;
	size_t openedBrackets = 0;
};

void AssertIsInputNotEmpty(std::string_view input)
{
	if (input.empty())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.message = "Входная строка для программы не может быть пустой",
			.expected = "Текст программы",
			.actual = std::string(input)});
	}
}

bool IsAtEnd(const LexerState& state)
{
	return state.pos >= state.source.length();
}

char Peek(const LexerState& state)
{
	if (IsAtEnd(state))
	{
		return '\0';
	}
	return state.source[state.pos];
}

char PeekNext(const LexerState& state)
{
	if (state.pos + 1 >= state.source.length())
	{
		return '\0';
	}
	return state.source[state.pos + 1];
}

char Advance(LexerState& state)
{
	const char ch = Peek(state);
	state.pos++;
	return ch;
}

void SkipWhitespacesAndComments(LexerState& state)
{
	while (!IsAtEnd(state))
	{
		const char ch = Peek(state);
		if (ch == ' ' || ch == '\t' || ch == '\r')
		{
			Advance(state);
		}
		else if (ch == '\n')
		{
			if (state.openedBrackets > 0)
			{
				state.line++;
				Advance(state);
			}
			else
			{
				break;
			}
		}
		else if (ch == '#')
		{
			while (!IsAtEnd(state) && Peek(state) != '\n')
			{
				Advance(state);
			}
		}
		else
		{
			break;
		}
	}
}

void SkipMultipleNewLines(LexerState& state)
{
	while (!IsAtEnd(state))
	{
		const char ch = Peek(state);
		if (ch == '\n')
		{
			state.line++;
			Advance(state);
		}
		else if (ch == ' ' || ch == '\t' || ch == '\r')
		{
			Advance(state);
		}
		else if (ch == '#')
		{
			while (!IsAtEnd(state) && Peek(state) != '\n')
			{
				Advance(state);
			}
		}
		else
		{
			break;
		}
	}
}

Token ParseNewLine(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;

	SkipMultipleNewLines(state);

	return Token{TokenType::NewLine, "\n", startPos, startLine};
}

[[noreturn]] void ReportUnknownSymbol(char symbol, size_t line, size_t pos)
{
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Lexer,
		.message = "Встречен неизвестный или недопустимый символ",
		.expected = "Валидный токен алфавита Weasel",
		.actual = std::string(1, symbol),
		.line = line,
		.pos = pos});
}

[[noreturn]] void ReportUnterminatedString(size_t line, size_t pos)
{
	throw CompilationException(DiagnosticData{
		.phase = CompilerPhase::Lexer,
		.message = "Незакрытая строковая константа",
		.expected = "\"",
		.actual = "Конец файла (EOF)",
		.line = line,
		.pos = pos});
}

bool IsIdStart(char ch)
{
	return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool IsIdChar(char ch)
{
	return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool IsKeyword(std::string_view text)
{
	return std::ranges::binary_search(LanguageTokens::AllKeywords, text);
}

Token ParseIdOrKeyword(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;

	while (!IsAtEnd(state) && IsIdChar(Peek(state)))
	{
		Advance(state);
	}

	const std::string_view value = state.source.substr(startPos, state.pos - startPos);
	const TokenType type = IsKeyword(value) ? TokenType::Keyword : TokenType::Identifier;

	return Token{type, value, startPos, startLine};
}

Token ParseNum(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;
	bool isFloat = false;

	while (!IsAtEnd(state) && std::isdigit(static_cast<unsigned char>(Peek(state))))
	{
		Advance(state);
	}

	if (Peek(state) == '.' && std::isdigit(static_cast<unsigned char>(PeekNext(state))))
	{
		isFloat = true;
		Advance(state);

		while (!IsAtEnd(state) && std::isdigit(static_cast<unsigned char>(Peek(state))))
		{
			Advance(state);
		}
	}

	const std::string_view value = state.source.substr(startPos, state.pos - startPos);
	const TokenType type = isFloat ? TokenType::Float : TokenType::Integer;

	return Token{type, value, startPos, startLine};
}

Token ParseString(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;

	Advance(state);
	const size_t contentStart = state.pos;

	while (!IsAtEnd(state) && Peek(state) != '"')
	{
		if (Peek(state) == '\n')
		{
			state.line++;
		}
		Advance(state);
	}

	const size_t contentEnd = state.pos;

	if (IsAtEnd(state))
	{
		ReportUnterminatedString(startLine, startPos);
	}

	Advance(state);

	const std::string_view value = state.source.substr(contentStart, contentEnd - contentStart);

	return Token{TokenType::String, value, startPos, startLine};
}

Token MakeToken(TokenType type, size_t startPos, size_t length, const LexerState& state, size_t line)
{
	return Token{type, state.source.substr(startPos, length), startPos, line};
}

void HandleBracketOpen(LexerState& state)
{
	state.openedBrackets++;
}

void HandleBracketClose(LexerState& state)
{
	if (state.openedBrackets > 0)
	{
		state.openedBrackets--;
	}
}

Token ParseOperatorOrPunctuation(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;
	const char ch = Advance(state);

	switch (ch)
	{
	case LanguageTokens::SymColon[0]:
		return MakeToken(TokenType::Colon, startPos, 1, state, startLine);
	case LanguageTokens::OpLess[0]:
		if (Peek(state) == LanguageTokens::OpMove[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpMove, startPos, 2, state, startLine);
		}
		if (Peek(state) == LanguageTokens::OpLessEq[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpLessEq, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::OpLess, startPos, 1, state, startLine);
	case LanguageTokens::OpGreater[0]:
		if (Peek(state) == LanguageTokens::OpNotEq[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpNotEq, startPos, 2, state, startLine);
		}
		if (Peek(state) == LanguageTokens::OpGreaterEq[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpGreaterEq, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::OpGreater, startPos, 1, state, startLine);
	case LanguageTokens::OpEq[0]:
		if (Peek(state) == LanguageTokens::OpEq[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpEq, startPos, 2, state, startLine);
		}
	case LanguageTokens::SymDot[0]:
		if (Peek(state) == LanguageTokens::OpRange[1])
		{
			Advance(state);
			return MakeToken(TokenType::OpRange, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::Dot, startPos, 1, state, startLine);
	case LanguageTokens::OpPlus[0]:
		return MakeToken(TokenType::OpPlus, startPos, 1, state, startLine);
	case LanguageTokens::OpMinus[0]:
		return MakeToken(TokenType::OpMinus, startPos, 1, state, startLine);
	case LanguageTokens::OpMul[0]:
		return MakeToken(TokenType::OpMul, startPos, 1, state, startLine);
	case LanguageTokens::OpDiv[0]:
		return MakeToken(TokenType::OpDiv, startPos, 1, state, startLine);
	case LanguageTokens::OpMod[0]:
		return MakeToken(TokenType::OpMod, startPos, 1, state, startLine);
	case LanguageTokens::OpAddressOf[0]:
		return MakeToken(TokenType::Ampersand, startPos, 1, state, startLine);
	case LanguageTokens::SymComma[0]:
		return MakeToken(TokenType::Comma, startPos, 1, state, startLine);
	case LanguageTokens::SymParenLeft[0]:
		HandleBracketOpen(state);
		return MakeToken(TokenType::ParenLeft, startPos, 1, state, startLine);
	case LanguageTokens::SymParenRight[0]:
		HandleBracketClose(state);
		return MakeToken(TokenType::ParenRight, startPos, 1, state, startLine);
	case LanguageTokens::SymBracketLeft[0]:
		HandleBracketOpen(state);
		return MakeToken(TokenType::BracketLeft, startPos, 1, state, startLine);
	case LanguageTokens::SymBracketRight[0]:
		HandleBracketClose(state);
		return MakeToken(TokenType::BracketRight, startPos, 1, state, startLine);
	case LanguageTokens::SymBraceLeft[0]:
		return MakeToken(TokenType::BraceLeft, startPos, 1, state, startLine);
	case LanguageTokens::SymBraceRight[0]:
		return MakeToken(TokenType::BraceRight, startPos, 1, state, startLine);
	default:
		ReportUnknownSymbol(ch, startLine, startPos);
	}
}

Token GetNextToken(LexerState& state)
{
	SkipWhitespacesAndComments(state);

	if (IsAtEnd(state))
	{
		return Token{TokenType::EndOfFile, "", state.pos, state.line};
	}

	const char ch = Peek(state);

	if (ch == '\n')
	{
		return ParseNewLine(state);
	}

	if (IsIdStart(ch))
	{
		return ParseIdOrKeyword(state);
	}

	if (std::isdigit(static_cast<unsigned char>(ch)))
	{
		return ParseNum(state);
	}

	if (ch == '"')
	{
		return ParseString(state);
	}

	return ParseOperatorOrPunctuation(state);
}

std::vector<Token> CollectTokens(LexerState& state)
{
	std::vector<Token> tokens;
	tokens.reserve(state.source.length() / AverageTokenLength);

	while (true)
	{
		auto token = GetNextToken(state);
		auto type = token.type;
		tokens.emplace_back(std::move(token));

		if (type == TokenType::EndOfFile)
		{
			break;
		}
	}

	return tokens;
}
} // namespace

std::vector<Token> Lexer::Tokenize(std::string_view input, DiagnosticEngine& engine)
{
	AssertIsInputNotEmpty(input);
	LexerState state{input, engine};
	return CollectTokens(state);
}