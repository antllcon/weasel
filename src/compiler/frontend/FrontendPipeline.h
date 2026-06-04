#pragma once
#include "src/compiler/ast/AstNode.h"
#include "src/compiler/codegen/CodegenContext.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/context/LanguageContext.h"
#include <filesystem>
#include <memory>
#include <optional>

namespace FrontendPipeline
{
struct FrontendResult
{
	std::unique_ptr<AstNode> ast;
	CodegenContext context;
};

[[nodiscard]] std::optional<FrontendResult> Run(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	DiagnosticEngine& engine);
} // namespace FrontendPipeline