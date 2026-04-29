#pragma once
#include <cstddef>

struct SourceLocation
{
	size_t line = 0;
	size_t pos = 0;
};

struct SourceRange
{
	SourceLocation start;
	SourceLocation end;
};