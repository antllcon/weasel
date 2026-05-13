#include "Lexer.h"
#include "src/diagnostics/CompilationException.h"

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
	DiagnosticEngine& engine;
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

void ReportUnknownSymbol(const LexerState& state, char symbol, size_t line, size_t pos)
{
	state.engine.Report(DiagnosticData{
		.phase = CompilerPhase::Lexer,
		.message = "Встречен неизвестный или недопустимый символ",
		.expected = "Валидный токен алфавита Weasel",
		.actual = std::string(1, symbol),
		.line = line,
		.pos = pos});
}

void ReportUnterminatedString(const LexerState& state, size_t line, size_t pos)
{
	state.engine.Report(DiagnosticData{
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
	constexpr std::array<std::string_view, 51> keywords = {
		"acolor", "als", "and", "ass", "bitten", "bnd", "bnt", "boolen", "bor", "buffer", "bxr", "compti", "def", "double", "else", "enumer", "false", "fpoint", "import", "in", "linear", "little", "longer", "matrix", "not", "number", "orr", "planar", "quadra", "random", "rep", "return", "rpoint", "run", "s", "single", "slices", "spoint", "string", "struct", "thread", "true", "u", "unions", "upoint", "val", "var", "vector", "voided", "when", "wpoint"};

	return std::ranges::binary_search(keywords, text);
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

	if (IsAtEnd(state))
	{
		ReportUnterminatedString(state, startLine, startPos);
	}
	else
	{
		Advance(state);
	}

	const std::string_view value = state.source.substr(startPos, state.pos - startPos);

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
		ReportUnknownSymbol(state, ch, startLine, startPos);
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
		HandleBracketOpen(state);
		return MakeToken(TokenType::ParenLeft, startPos, 1, state, startLine);
	case ')':
		HandleBracketClose(state);
		return MakeToken(TokenType::ParenRight, startPos, 1, state, startLine);
	case '[':
		HandleBracketOpen(state);
		return MakeToken(TokenType::BracketLeft, startPos, 1, state, startLine);
	case ']':
		HandleBracketClose(state);
		return MakeToken(TokenType::BracketRight, startPos, 1, state, startLine);
	case '{':
		return MakeToken(TokenType::BraceLeft, startPos, 1, state, startLine);
	case '}':
		return MakeToken(TokenType::BraceRight, startPos, 1, state, startLine);
	default:
		ReportUnknownSymbol(state, ch, startLine, startPos);
		break;
	}

	return MakeToken(TokenType::Error, startPos, 1, state, startLine);
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
		return ParseNumber(state);
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