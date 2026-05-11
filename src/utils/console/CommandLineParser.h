#pragma once

#include <filesystem>
#include <string_view>

constexpr std::string_view DEFAULT_GRAMMAR_FILE = "grammar.wesg";

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
	LogTarget logTarget{LogTarget::None};
	bool emitNasm{false};
};

namespace CommandLineParser
{
[[nodiscard]] CompilerOptions Parse(int argc, char* argv[]);
}