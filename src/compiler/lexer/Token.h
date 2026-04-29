#pragma once
#include <string_view>

enum class TokenType
{
	Identifier,
	Integer,
	Float,
	String,
	Keyword,

	OpAssign,
	OpMove,
	OpEq,
	OpNotEq,
	OpLessEq,
	OpGreaterEq,
	OpLess,
	OpGreater,
	OpPlus,
	OpMinus,
	OpMul,
	OpDiv,
	OpMod,
	OpShiftLeft,
	OpShiftRight,
	OpBitAnd,
	OpRange,

	Comma,
	Dot,
	Colon,
	Semicolon,
	At,
	ParenLeft,
	ParenRight,
	BracketLeft,
	BracketRight,
	BraceLeft,
	BraceRight,

	EndOfFile,
	Error
};

struct Token
{
	TokenType type;
	std::string_view value;
	size_t pos;
	size_t line;
};