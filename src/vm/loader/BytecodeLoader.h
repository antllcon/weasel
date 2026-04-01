#pragma once
#include "src/vm/chunk/Chunk.h"
#include <filesystem>

class BytecodeLoader
{
public:
	[[nodiscard]] static Chunk LoadFile(const std::filesystem::path& filePath);
};