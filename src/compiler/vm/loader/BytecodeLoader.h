#pragma once
#include "src/compiler/vm/chunk/Chunk.h"
#include <filesystem>

class BytecodeLoader
{
public:
	[[nodiscard]] static Chunk LoadFile(const std::filesystem::path& filePath);
};