#pragma once
#include "src/diagnostics/DiagnosticEngine.h"

namespace SourceLoader
{
std::string Read(const std::filesystem::path& filePath);
} // namespace SourceLoader