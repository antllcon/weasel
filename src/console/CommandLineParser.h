#pragma once

#include <filesystem>

enum class LogTarget
{
	None,
	Console,
	File
};

struct CompilerOptions
{
	std::filesystem::path sourceFile;
	std::filesystem::path grammarFile = "grammar.wesg";
	LogTarget logTarget = LogTarget::None;
};

namespace CommandLineParser
{
[[nodiscard]] CompilerOptions Parse(int argc, char* argv[]);
}