#pragma once
#include <filesystem>
#include <string>

enum class CompilerPhase
{
	Lexer,
	Parser,
	Semantic,
	Optimizer,
	Backend,
	VirtualMachine
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