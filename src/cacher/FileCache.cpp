#include "FileCache.h"
#include <fstream>
#include <stdexcept>

namespace
{
constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

uint64_t Fnv1a64(const std::string& data)
{
	uint64_t hash = FNV_OFFSET;
	for (unsigned char c : data)
	{
		hash ^= c;
		hash *= FNV_PRIME;
	}
	return hash;
}

std::string ReadFileContent(const std::filesystem::path& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file)
	{
		throw std::runtime_error("Не удалось открыть файл для хэширования");
	}
	return std::string(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
}

void WriteUint64(std::ofstream& out, uint64_t value)
{
	out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint64_t ReadUint64(std::ifstream& in)
{
	uint64_t value = 0;
	in.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}
} // namespace

namespace FileCache
{
uint64_t ComputeHash(const std::filesystem::path& filePath)
{
	return Fnv1a64(ReadFileContent(filePath));
}

bool IsUpToDate(const std::filesystem::path& sourcePath, const std::filesystem::path& cachePath)
{
	if (!std::filesystem::exists(cachePath))
	{
		return false;
	}

	std::ifstream cacheFile(cachePath, std::ios::binary);
	if (!cacheFile)
	{
		return false;
	}

	const auto storedHash = ReadUint64(cacheFile);
	if (!cacheFile)
	{
		return false;
	}

	try
	{
		const auto currentHash = ComputeHash(sourcePath);
		return storedHash == currentHash;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<uint8_t> ReadPayload(const std::filesystem::path& cachePath)
{
	std::ifstream cacheFile(cachePath, std::ios::binary);
	if (!cacheFile)
	{
		throw std::runtime_error("Не удалось открыть кэш-файл для чтения");
	}

	cacheFile.seekg(sizeof(uint64_t), std::ios::beg);

	return std::vector<uint8_t>(
		std::istreambuf_iterator(cacheFile),
		std::istreambuf_iterator<char>());
}

void Write(const std::filesystem::path& sourcePath,
	const std::filesystem::path& cachePath,
	const std::vector<uint8_t>& payload)
{
	const auto hash = ComputeHash(sourcePath);

	std::ofstream cacheFile(cachePath, std::ios::binary | std::ios::trunc);
	if (!cacheFile)
	{
		throw std::runtime_error("Не удалось открыть кэш-файл для записи");
	}

	WriteUint64(cacheFile, hash);
	cacheFile.write(reinterpret_cast<const char*>(payload.data()),
		static_cast<std::streamsize>(payload.size()));
}
} // namespace FileCache