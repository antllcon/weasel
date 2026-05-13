#pragma once

#include "Token.h"
#include "src/diagnostics/Diagnostic.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include <string_view>
#include <vector>

class Lexer
{
public:
	[[nodiscard]] static std::vector<Token> Tokenize(std::string_view input, DiagnosticEngine& engine);
};