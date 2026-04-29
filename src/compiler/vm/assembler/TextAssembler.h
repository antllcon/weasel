#pragma once
#include <filesystem>

class TextAssembler
{
public:
	static void AssembleToBinary(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath);
};