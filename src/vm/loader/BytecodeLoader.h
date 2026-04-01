#pragma once
#include "src/vm/chank/Chunk.h"
#include <filesystem>

class BytecodeLoader
{
public:
	[[nodiscard]] static Chunk LoadFile(const std::filesystem::path& filePath);
};