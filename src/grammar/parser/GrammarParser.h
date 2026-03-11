#pragma once
#include "src/grammar/GrammarTypes.h"
#include <filesystem>
#include <string>

class GrammarParser
{
public:
	[[nodiscard]] static raw::Rules ParseFile(const std::filesystem::path& path);
	[[nodiscard]] static raw::Rules ParseString(const std::string& content);
};