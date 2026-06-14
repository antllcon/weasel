#pragma once

#include "src/compiler/lexer/Token.h"
#include "src/utils/console/CommandLineParser.h"
#include <string>
#include <vector>

class Formatter
{
public:
	static bool FormatFile(const CompilerOptions& options);
	static std::string Format(const std::vector<Token>& tokens);
};