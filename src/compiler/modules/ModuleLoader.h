#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/context/LanguageContext.h"
#include <filesystem>
#include <memory>

class ModuleLoader
{
public:
	[[nodiscard]] static std::unique_ptr<AstNode> Load(
		const std::filesystem::path& rootFile,
		const std::filesystem::path& stdlibDir,
		const LanguageContext& context,
		DiagnosticEngine& engine);
};
