#pragma once
#include "token/Token.h"
#include <string_view>
#include <vector>

class Lexer
{
public:
	[[nodiscard]] static std::vector<Token> Tokenize(std::string_view input);
};