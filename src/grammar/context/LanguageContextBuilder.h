#pragma once

#include "LanguageContext.h"
#include <filesystem>

namespace LanguageContextBuilder
{
[[nodiscard]] LanguageContext Build(const std::filesystem::path& grammarFile);
}