#pragma once

#include "src/diagnostics/DiagnosticEngine.h"
#include "src/grammar/lalrTableBuilder/LalrTypes.h"
#include "src/logger/ILogger.h"
#include <filesystem>
#include <memory>
#include <string>

class CompilerPipeline
{
public:
	void InitGrammar(const std::filesystem::path& grammarFile, const std::shared_ptr<ILogger>& logger);
	bool Compile(const std::filesystem::path& sourceFile, const std::shared_ptr<ILogger>& logger);

private:
	DiagnosticEngine m_engine;
	std::string m_startSymbol;
	std::unique_ptr<LalrTable> m_lalrTable;
};