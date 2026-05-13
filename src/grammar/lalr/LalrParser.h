#pragma once
#include "src/compiler/lexer/Token.h"
#include "src/grammar/cst/CstNode.h"
#include "src/grammar/lalr/LalrTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace LalrParser
{
[[nodiscard]] std::unique_ptr<CstNode> ParseToTree(
	const LalrTable& table,
	const std::vector<CstInputToken>& tokens);

[[nodiscard]] std::vector<LalrParseStep> ParseToSteps(
	const LalrTable& table,
	const std::vector<std::string>& tokens);

std::unique_ptr<CstNode> ParseTokenStream(
	const LalrTable& table,
	const std::vector<Token>& tokens,
	bool logSteps = false);
} // namespace LalrParser