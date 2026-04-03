#include "Lexer.h"

#include "src/diagnostics/CompilationException.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

namespace
{
constexpr size_t AVERAGE_TOKEN_LENGTH = 5;
constexpr size_t INITIAL_POSITION = 0;
constexpr size_t INITIAL_LINE = 1;

struct LexerState
{
	std::string_view source;
	size_t pos = INITIAL_POSITION;
	size_t line = INITIAL_LINE;
};

void AssertIsInputNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "LEX-001",
			.message = "Входная строка для лексера не может быть пустой",
			.expected = "Текст программы",
			.actual = "Пустая строка"});
	}
}

void AssertIsUnknownSymbol(bool isUnknown, char symbol, size_t line, size_t pos)
{
	if (isUnknown)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "LEX-002",
			.message = "Встречен неизвестный или недопустимый символ",
			.expected = "Валидный токен алфавита Weasel",
			.actual = std::string(1, symbol),
			.line = line,
			.pos = pos});
	}
}

void AssertIsStringTerminated(bool isTerminated, size_t line, size_t pos)
{
	if (!isTerminated)
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.errorCode = "LEX-003",
			.message = "Незакрытая строковая константа",
			.expected = "\"",
			.actual = "Конец файла (EOF)",
			.line = line,
			.pos = pos});
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
			state.line++;
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
	constexpr std::array<std::string_view, 50> keywords = {
		"val", "var", "def", "s", "u", "bitten", "little", "number", "longer", "single", "double", "boolen", "string", "voided", "planar", "vector", "quadra", "linear", "matrix", "slices", "buffer", "rpoint", "fpoint", "struct", "unions", "enumer", "thread", "wpoint", "spoint", "upoint", "random", "when", "else", "run", "rep", "in", "and", "orr", "not", "bnd", "bor", "bxr", "bnt", "import", "ass", "als", "return", "true", "false", "compti"};

	return std::ranges::find(keywords, text) != keywords.end();
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

Token ParseNumber(LexerState& state)
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

	while (!IsAtEnd(state) && Peek(state) != '"')
	{
		if (Peek(state) == '\n')
		{
			state.line++;
		}
		Advance(state);
	}

	AssertIsStringTerminated(!IsAtEnd(state), state.line, state.pos);

	Advance(state);

	const std::string_view value = state.source.substr(startPos, state.pos - startPos);

	return Token{TokenType::String, value, startPos, startLine};
}

Token MakeToken(TokenType type, size_t startPos, size_t length, const LexerState& state, size_t line)
{
	return Token{type, state.source.substr(startPos, length), startPos, line};
}

Token ParseOperatorOrPunctuation(LexerState& state)
{
	const size_t startPos = state.pos;
	const size_t startLine = state.line;
	const char ch = Advance(state);

	switch (ch)
	{
	case ':':
		if (Peek(state) == '=')
		{
			Advance(state);
			return MakeToken(TokenType::OpAssign, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::Colon, startPos, 1, state, startLine);
	case '<':
		if (Peek(state) == '-')
		{
			Advance(state);
			return MakeToken(TokenType::OpMove, startPos, 2, state, startLine);
		}
		if (Peek(state) == '=')
		{
			Advance(state);
			return MakeToken(TokenType::OpLessEq, startPos, 2, state, startLine);
		}
		if (Peek(state) == '<')
		{
			Advance(state);
			return MakeToken(TokenType::OpShiftLeft, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::OpLess, startPos, 1, state, startLine);
	case '>':
		if (Peek(state) == '<')
		{
			Advance(state);
			return MakeToken(TokenType::OpNotEq, startPos, 2, state, startLine);
		}
		if (Peek(state) == '=')
		{
			Advance(state);
			return MakeToken(TokenType::OpGreaterEq, startPos, 2, state, startLine);
		}
		if (Peek(state) == '>')
		{
			Advance(state);
			return MakeToken(TokenType::OpShiftRight, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::OpGreater, startPos, 1, state, startLine);
	case '=':
		if (Peek(state) == '=')
		{
			Advance(state);
			return MakeToken(TokenType::OpEq, startPos, 2, state, startLine);
		}
		AssertIsUnknownSymbol(true, ch, startLine, startPos);
		break;
	case '.':
		if (Peek(state) == '.')
		{
			Advance(state);
			return MakeToken(TokenType::OpRange, startPos, 2, state, startLine);
		}
		return MakeToken(TokenType::Dot, startPos, 1, state, startLine);
	case '+':
		return MakeToken(TokenType::OpPlus, startPos, 1, state, startLine);
	case '-':
		return MakeToken(TokenType::OpMinus, startPos, 1, state, startLine);
	case '*':
		return MakeToken(TokenType::OpMul, startPos, 1, state, startLine);
	case '/':
		return MakeToken(TokenType::OpDiv, startPos, 1, state, startLine);
	case '%':
		return MakeToken(TokenType::OpMod, startPos, 1, state, startLine);
	case '&':
		return MakeToken(TokenType::OpBitAnd, startPos, 1, state, startLine);
	case ',':
		return MakeToken(TokenType::Comma, startPos, 1, state, startLine);
	case '(':
		return MakeToken(TokenType::ParenLeft, startPos, 1, state, startLine);
	case ')':
		return MakeToken(TokenType::ParenRight, startPos, 1, state, startLine);
	case '[':
		return MakeToken(TokenType::BracketLeft, startPos, 1, state, startLine);
	case ']':
		return MakeToken(TokenType::BracketRight, startPos, 1, state, startLine);
	case '{':
		return MakeToken(TokenType::BraceLeft, startPos, 1, state, startLine);
	case '}':
		return MakeToken(TokenType::BraceRight, startPos, 1, state, startLine);
	default:
		AssertIsUnknownSymbol(true, ch, startLine, startPos);
	}

	return Token{TokenType::Error, "", startPos, startLine};
}

Token GetNextToken(LexerState& state)
{
	SkipWhitespacesAndComments(state);

	if (IsAtEnd(state))
	{
		return Token{TokenType::EndOfFile, "", state.pos, state.line};
	}

	const char ch = Peek(state);

	if (IsIdStart(ch))
	{
		return ParseIdOrKeyword(state);
	}

	if (std::isdigit(static_cast<unsigned char>(ch)))
	{
		return ParseNumber(state);
	}

	if (ch == '"')
	{
		return ParseString(state);
	}

	return ParseOperatorOrPunctuation(state);
}
} // namespace

std::vector<Token> Lexer::Tokenize(std::string_view input)
{
	AssertIsInputNotEmpty(input.empty());

	LexerState state{input};
	std::vector<Token> tokens;

	const size_t estimatedTokens = input.length() / AVERAGE_TOKEN_LENGTH;
	tokens.reserve(estimatedTokens);

	bool isEof = false;

	while (!isEof)
	{
		Token token = GetNextToken(state);

		if (token.type == TokenType::EndOfFile)
		{
			isEof = true;
		}

		tokens.push_back(token);
	}

	return tokens;
}