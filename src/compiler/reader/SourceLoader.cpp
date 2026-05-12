#include "SourceLoader.h"
#include "src/diagnostics/CompilationException.h"
#include <fstream>

namespace
{
void AssertIsFileExists(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.message = "Файл не найден",
			.actual = filePath.string()});
	}
}

void AssertIsFileOpened(const std::ifstream& file, const std::filesystem::path& filePath)
{
	if (!file.is_open())
	{
		throw CompilationException(DiagnosticData{
			.phase = CompilerPhase::Lexer,
			.message = "Не удалось открыть файл для чтения",
			.actual = filePath.string()});
	}
}
} // namespace

std::string SourceLoader::Read(const std::filesystem::path& filePath)
{
	AssertIsFileExists(filePath);
	std::ifstream file(filePath, std::ios::in | std::ios::binary);

	AssertIsFileOpened(file, filePath);
	std::ostringstream content;
	content << file.rdbuf();

	return content.str();
}
