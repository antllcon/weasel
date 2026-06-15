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

constexpr std::string_view DEFAULT_STDLIB_DIR = "stdlib";

struct CompilerOptions
{
	std::filesystem::path sourceFile;
	std::filesystem::path grammarFile{DEFAULT_GRAMMAR_FILE};
	std::filesystem::path stdlibDir{DEFAULT_STDLIB_DIR};
	LogTarget logTarget{LogTarget::None};
	bool emitNasm{false};
	bool formatMode{false};
	bool checkOnly{false};
};

namespace CommandLineParser
{
[[nodiscard]] CompilerOptions Parse(int argc, char* argv[]);
}