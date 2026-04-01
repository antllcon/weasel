#pragma once
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
	std::string errorCode;
	std::string message;
	std::string expected;
	std::string actual;
	size_t line = 0;
	size_t pos = 0;
	std::string filePath;
};