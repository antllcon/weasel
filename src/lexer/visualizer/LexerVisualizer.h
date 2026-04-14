#pragma once

#include "src/diagnostics/Diagnostic.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/lexer/Token.h"
#include <string_view>
#include <vector>

class LexerVisualizer
{
public:
	static void Visualize(
		const std::vector<Token>& tokens,
		const DiagnosticEngine& engine);
};