#pragma once

#include <filesystem>

namespace
{
constexpr std::string_view DEFAULT_GRAMMAR_FILE = "grammar.wesg";
}

enum class LogTarget
{
	None = 0,
	Console,
	File
};

struct CompilerOptions
{
	std::filesystem::path sourceFile;
	std::filesystem::path grammarFile{DEFAULT_GRAMMAR_FILE};
	LogTarget logTarget = LogTarget::None;
};

namespace CommandLineParser
{
[[nodiscard]] CompilerOptions Parse(int argc, char* argv[]);
}