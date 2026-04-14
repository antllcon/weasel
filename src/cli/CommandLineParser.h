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
	std::filesystem::path grammarFile = "grammar.txt";
	LogTarget logTarget = LogTarget::None;
};

class CommandLineParser
{
public:
	[[nodiscard]] static CompilerOptions Parse(int argc, char* argv[]);
};