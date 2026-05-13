#pragma once

#include "src/compiler/lexer/Token.h"
#include "src/diagnostics/DiagnosticEngine.h"

class LexerVisualizer
{
public:
	static void Visualize(
		const std::vector<Token>& tokens,
		const DiagnosticEngine& engine,
		bool splitBySourceLines = false);
};