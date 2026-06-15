#pragma once
#include <filesystem>
#include <string>

struct WkitConfig
{
	std::string name;
	std::string entry;
	std::string stdlib;
};

namespace WkitConfigLoader
{
[[nodiscard]] WkitConfig Load(const std::filesystem::path& configFile);
}
