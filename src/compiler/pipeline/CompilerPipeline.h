#pragma once

#include "src/grammar/context/LanguageContext.h"
#include "src/utils/console/CommandLineParser.h"

namespace CompilerPipeline
{
[[nodiscard]] bool Compile(const CompilerOptions& options, const LanguageContext& context);
}