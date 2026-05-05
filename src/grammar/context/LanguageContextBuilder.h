#pragma once

#include "LanguageContext.h"
#include "src/utils/logger/ILogger.h"
#include <filesystem>
#include <memory>

namespace LanguageContextBuilder
{
[[nodiscard]] LanguageContext Build(
	const std::filesystem::path& grammarFile,
	const std::shared_ptr<ILogger>& logger);
}