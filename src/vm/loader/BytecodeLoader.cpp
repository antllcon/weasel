#include "BytecodeLoader.h"
#include "src/vm/chank/Chunk.h"
#include <fstream>
#include <stdexcept>

namespace
{
void AssertIsFileOpened(bool isOpen)
{
	if (!isOpen)
	{
		throw std::runtime_error("Не удалось открыть бинарный файл байт-кода");
	}
}

void AssertIsMagicValid(uint32_t magic)
{
	if (magic != 0x314C5357)
	{
		throw std::runtime_error("Неверный формат файла или неподдерживаемая версия байт-кода");
	}
}

void AssertIsStringLengthValid(uint32_t length)
{
	if (length > 1048576)
	{
		throw std::runtime_error("Превышен максимально допустимый размер строки в пуле");
	}
}

uint8_t ReadUint8(std::ifstream& stream)
{
	uint8_t value = 0;
	stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

uint32_t ReadUint32(std::ifstream& stream)
{
	uint32_t value = 0;
	stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

uint64_t ReadUint64(std::ifstream& stream)
{
	uint64_t value = 0;
	stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

void LoadConstants(std::ifstream& file, Chunk& chunk)
{
	const uint32_t count = ReadUint32(file);
	for (uint32_t i = 0; i < count; ++i)
	{
		const uint64_t rawValue = ReadUint64(file);
		chunk.AddConstant(Value(rawValue));
	}
}

void LoadStrings(std::ifstream& file, Chunk& chunk)
{
	const uint32_t count = ReadUint32(file);
	for (uint32_t i = 0; i < count; ++i)
	{
		const uint32_t length = ReadUint32(file);
		AssertIsStringLengthValid(length);

		std::string text(length, '\0');
		if (length > 0)
		{
			file.read(text.data(), length);
		}
		chunk.AddString(text);
	}
}

void LoadCode(std::ifstream& file, Chunk& chunk)
{
	const uint32_t codeSize = ReadUint32(file);
	for (uint32_t i = 0; i < codeSize; ++i)
	{
		const auto opCode = static_cast<OpCode>(ReadUint8(file));
		const uint32_t line = ReadUint32(file);
		chunk.WriteOpCode(opCode, line);
	}
}
} // namespace

Chunk BytecodeLoader::LoadFile(const std::filesystem::path& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	AssertIsFileOpened(file.is_open());

	const uint32_t magic = ReadUint32(file);
	AssertIsMagicValid(magic);

	Chunk chunk;
	LoadConstants(file, chunk);
	LoadStrings(file, chunk);
	LoadCode(file, chunk);

	return chunk;
}