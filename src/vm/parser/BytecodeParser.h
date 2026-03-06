#pragma once
#include "src/vm/chank/Chunk.h"
#include <filesystem>

class BytecodeParser
{
public:
	[[nodiscard]] static Chunk ParseFile(const std::filesystem::path& filePath);
};