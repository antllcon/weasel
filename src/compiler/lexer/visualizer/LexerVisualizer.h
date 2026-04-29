#pragma once
#include "src/compiler/lexer/Token.h"
#include "src/diagnostics/Diagnostic.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include <vector>

class LexerVisualizer
{
public:
	static void Visualize(
		const std::vector<Token>& tokens,
		const DiagnosticEngine& engine);
};