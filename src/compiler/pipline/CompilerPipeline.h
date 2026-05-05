#pragma once

#include "src/grammar/context/LanguageContext.h"
#include <filesystem>

namespace CompilerPipeline
{
[[nodiscard]] bool Compile(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context);
}