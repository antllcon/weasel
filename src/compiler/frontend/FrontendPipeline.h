#pragma once

#include "src/compiler/ast/AstNode.h"
#include "src/compiler/sema/SemanticAnalyzer.h"
#include "src/compiler/sema/SymbolTable.h"
#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/context/LanguageContext.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace FrontendPipeline
{

struct FrontendResult
{
	std::unique_ptr<AstNode> ast;
	std::unordered_map<const AstNode*, SymbolInfo> symbols;
	std::unordered_map<const AstNode*, uint32_t> varDeclSlots;
	std::unordered_map<const AstNode*, std::vector<SymbolInfo>> repIterators;
	std::unordered_map<std::string, SemanticAnalyzer::FunctionInfo> functions;
};

[[nodiscard]] std::optional<FrontendResult> Run(
	const std::filesystem::path& sourceFile,
	const LanguageContext& context,
	DiagnosticEngine& engine);

} // namespace FrontendPipeline