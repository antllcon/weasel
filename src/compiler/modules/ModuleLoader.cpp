#include "ModuleLoader.h"

#include "src/compiler/ast/ImportDecl.h"
#include "src/compiler/ast/ProgramNode.h"
#include "src/compiler/cst-to-ast/CstToAstConverter.h"
#include "src/compiler/lexer/Lexer.h"
#include "src/compiler/reader/SourceLoader.h"
#include "src/grammar/lalr/LalrParser.h"

#include <set>

namespace
{
constexpr std::string_view ModuleExtension = ".wes";

struct LoadState
{
	std::filesystem::path stdlibDir;
	const LanguageContext& context;
	DiagnosticEngine& engine;
	std::set<std::filesystem::path> visited;
	std::vector<std::unique_ptr<AstNode>> declarations;
};

std::filesystem::path WithExtension(const std::filesystem::path& path)
{
	if (path.has_extension())
	{
		return path;
	}
	return std::filesystem::path(path).concat(ModuleExtension);
}

std::filesystem::path ResolveModulePath(
	const std::filesystem::path& importingDir,
	const std::filesystem::path& stdlibDir,
	const std::string& modulePath)
{
	const auto relative = WithExtension(modulePath);

	const auto localCandidate = importingDir / relative;
	if (std::filesystem::exists(localCandidate))
	{
		return localCandidate;
	}

	const auto stdlibCandidate = stdlibDir / relative;
	if (std::filesystem::exists(stdlibCandidate))
	{
		return stdlibCandidate;
	}

	return {};
}

void ReportModuleNotFound(LoadState& state, const ImportDecl& import, const std::filesystem::path& importingFile)
{
	state.engine.Report(DiagnosticData{
		.phase = CompilerPhase::Parser,
		.message = "Модуль не найден: " + import.GetModulePath(),
		.actual = import.GetModulePath(),
		.line = import.GetRange().start.line,
		.pos = import.GetRange().start.pos,
		.filePath = importingFile});
}

std::vector<std::unique_ptr<AstNode>> ParseFileDeclarations(const std::filesystem::path& file, LoadState& state)
{
	const std::string source = SourceLoader::Read(file);
	const auto tokens = Lexer::Tokenize(source, state.engine);
	const auto cstRoot = LalrParser::ParseTokenStream(*state.context.lalrTable, tokens, false);
	return CstToAstConverter::ConvertDeclarations(*cstRoot);
}

void LoadFile(const std::filesystem::path& file, LoadState& state)
{
	const auto canonical = std::filesystem::weakly_canonical(file);
	if (state.visited.contains(canonical))
	{
		return;
	}
	state.visited.insert(canonical);

	auto declarations = ParseFileDeclarations(file, state);
	const auto importingDir = file.parent_path();

	for (auto& declaration : declarations)
	{
		if (const auto* import = dynamic_cast<const ImportDecl*>(declaration.get()))
		{
			const auto resolved = ResolveModulePath(importingDir, state.stdlibDir, import->GetModulePath());
			if (resolved.empty())
			{
				ReportModuleNotFound(state, *import, file);
				continue;
			}
			LoadFile(resolved, state);
			continue;
		}

		state.declarations.push_back(std::move(declaration));
	}
}
} // namespace

std::unique_ptr<AstNode> ModuleLoader::Load(
	const std::filesystem::path& rootFile,
	const std::filesystem::path& stdlibDir,
	const LanguageContext& context,
	DiagnosticEngine& engine)
{
	LoadState state{stdlibDir, context, engine, {}, {}};
	LoadFile(rootFile, state);
	return std::make_unique<ProgramNode>(std::move(state.declarations));
}
