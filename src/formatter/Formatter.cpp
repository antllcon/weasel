#include "Formatter.h"
#include "src/compiler/core/LanguageTokens.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/reader/SourceLoader.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/utils/logger/Logger.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
bool IsBinaryOperator(TokenType type)
{
	return type == TokenType::OpAssign || type == TokenType::OpMove || type == TokenType::OpEq || type == TokenType::OpNotEq || type == TokenType::OpLessEq || type == TokenType::OpGreaterEq || type == TokenType::OpLess || type == TokenType::OpGreater || type == TokenType::OpPlus || type == TokenType::OpMinus || type == TokenType::OpMul || type == TokenType::OpDiv || type == TokenType::OpMod || type == TokenType::OpArrow || type == TokenType::OpRange;
}

bool IsTextBinaryOperator(std::string_view value)
{
	return value == LanguageTokens::KwAnd || value == LanguageTokens::KwOr || value == LanguageTokens::KwIn || value == LanguageTokens::KwStep || value == LanguageTokens::KwDown;
}

bool NeedsSpaceAfter(TokenType type, std::string_view value)
{
	if (type == TokenType::Comma || type == TokenType::Colon) return true;

	if (type == TokenType::Keyword)
	{
		if (value == LanguageTokens::KwNot || value == LanguageTokens::KwReturn || value == LanguageTokens::KwImport)
			return true;
	}
	return false;
}

bool IsControlKeyword(std::string_view value)
{
	return value == LanguageTokens::KwIf || value == LanguageTokens::KwWhen || value == LanguageTokens::KwRep || value == LanguageTokens::KwRun;
}
} // namespace

bool Formatter::FormatFile(const CompilerOptions& options)
{
	DiagnosticEngine engine;
	std::string source = SourceLoader::Read(options.sourceFile);
	auto tokens = Lexer::Tokenize(source, engine);

	if (engine.HasErrors())
	{
		for (const auto& diag : engine.GetDiagnostics())
		{
			std::cerr << DiagnosticEngine::FormatMessage(diag) << std::endl;
		}
		return false;
	}

	std::string formattedCode = Format(tokens);

	std::ofstream outFile(options.sourceFile, std::ios::trunc);
	if (!outFile.is_open())
	{
		std::cerr << "Не удалось открыть файл для записи: " << options.sourceFile.string() << std::endl;
		return false;
	}

	outFile << formattedCode;

	Logger::Log("Файл успешно отформатирован: " + options.sourceFile.string());
	return true;
}

std::string Formatter::Format(const std::vector<Token>& tokens)
{
	std::ostringstream out;
	int indentLevel = 0;
	bool needsIndent = true;

	auto applyIndent = [&]() {
		if (needsIndent)
		{
			for (int i = 0; i < indentLevel; ++i)
				out << "    ";
			needsIndent = false;
		}
	};

	for (size_t i = 0; i < tokens.size(); ++i)
	{
		const auto& token = tokens[i];
		const auto type = token.type;
		const auto value = token.value;

		if (type == TokenType::EndOfFile)
			break;

		if (type == TokenType::NewLine)
		{
			if (i > 0 && tokens[i - 1].type == TokenType::NewLine)
			{
				if (i > 1 && tokens[i - 2].type == TokenType::NewLine)
					continue;
			}
			out << "\n";
			needsIndent = true;
			continue;
		}

		if (type == TokenType::BraceRight)
		{
			indentLevel = std::max(0, indentLevel - 1);
		}

		applyIndent();

		if (type == TokenType::BraceLeft)
		{
			if (i > 0 && tokens[i - 1].type != TokenType::NewLine)
				out << " ";
			out << "{\n";
			indentLevel++;
			needsIndent = true;
			continue;
		}

		if (type == TokenType::BraceRight)
		{
			out << "}";
			continue;
		}

		if (IsBinaryOperator(type))
		{
			out << " " << value << " ";
			continue;
		}

		if (type == TokenType::Keyword && IsTextBinaryOperator(value))
		{
			out << " " << value << " ";
			continue;
		}

		out << value;

		if (NeedsSpaceAfter(type, value))
		{
			out << " ";
		}
		else if (i + 1 < tokens.size())
		{
			const auto nextType = tokens[i + 1].type;
			const auto nextValue = tokens[i + 1].value;

			bool isCurrentText = type == TokenType::Keyword || type == TokenType::Identifier || type == TokenType::Integer || type == TokenType::Float || type == TokenType::String;

			bool isNextText = nextType == TokenType::Keyword || nextType == TokenType::Identifier || nextType == TokenType::Integer || nextType == TokenType::Float || nextType == TokenType::String;

			if (isCurrentText && isNextText)
			{
				out << " ";
			}
			else if (type == TokenType::Identifier && nextType == TokenType::BracketLeft && i > 0 && tokens[i - 1].value == LanguageTokens::KwEnum)
			{
				out << " ";
			}
			else if (type == TokenType::Keyword && IsControlKeyword(value) && nextType == TokenType::ParenLeft)
			{
				out << " ";
			}
			else if (nextType == TokenType::BraceLeft && type != TokenType::NewLine)
			{
				out << " ";
			}
		}
	}

	return out.str();
}