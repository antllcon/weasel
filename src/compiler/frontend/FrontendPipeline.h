#pragma once

#include "src/compiler/ast/AstNode.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/context/LanguageContext.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace FrontendPipline
{

struct FrontendResult
{
	std::unique_ptr<AstNode> ast;
	std::unordered_map<std::string, uint32_t> slotMap;
};

[[nodiscard]] std::optional<FrontendResult> Run(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	DiagnosticEngine& engine);

} // namespace FrontendPipline