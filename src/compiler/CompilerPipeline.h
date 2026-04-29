#pragma once

#include "src/grammar/context/LanguageContext.h"
#include "src/logger/ILogger.h"
#include <filesystem>
#include <memory>

namespace CompilerPipeline
{
[[nodiscard]] bool Compile(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	const std::shared_ptr<ILogger>& logger);
}