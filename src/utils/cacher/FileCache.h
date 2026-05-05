#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

namespace FileCache
{
uint64_t ComputeHash(const std::filesystem::path& filePath);
bool IsUpToDate(const std::filesystem::path& sourcePath, const std::filesystem::path& cachePath);
std::vector<uint8_t> ReadPayload(const std::filesystem::path& cachePath);
void Write(const std::filesystem::path& sourcePath,
	const std::filesystem::path& cachePath,
	const std::vector<uint8_t>& payload);
} // namespace FileCache