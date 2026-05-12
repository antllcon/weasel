#pragma once
#include <filesystem>
#include <string>

enum class CompilerPhase
{
	Reader,
	Lexer,
	Parser,
	Semantic,
	Optimizer,
	Backend,
	VirtualMachine,
	Fatal
};

struct DiagnosticData
{
	CompilerPhase phase;
	std::string message;
	std::string expected;
	std::string actual;
	size_t line = 0;
	size_t pos = 0;
	std::filesystem::path filePath;
};