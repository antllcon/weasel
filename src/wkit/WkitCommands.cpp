#include "WkitCommands.h"
#include "WkitConfig.h"

#include "src/compiler/pipeline/CompilerPipeline.h"
#include "src/formatter/Formatter.h"
#include "src/grammar/context/LanguageContextBuilder.h"
#include "src/utils/console/CommandLineParser.h"
#include "src/utils/logger/Logger.h"
#include "src/utils/logger/LoggerFactory.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
namespace fs = std::filesystem;

constexpr std::string_view ConfigFileName = "wkit.wes";
constexpr std::string_view GrammarFileName = "grammar.wesg";
constexpr std::string_view StdlibDirName = "stdlib";

fs::path GetExecutableDir()
{
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	if (length > 0 && length < static_cast<DWORD>(MAX_PATH))
	{
		return fs::path(std::wstring(buffer, length)).parent_path();
	}
#endif
	return fs::current_path();
}

void WriteFile(const fs::path& path, const std::string& content)
{
	std::ofstream out(path, std::ios::trunc | std::ios::binary);
	if (!out.is_open())
	{
		throw std::runtime_error("Не удалось создать файл: " + path.string());
	}
	out << content;
}

std::string ConfigTemplate(const std::string& projectName)
{
	return "# Project config\n"
		   "let string name = \"" + projectName + "\"\n"
		   "let string entry = \"src/main.wes\"\n"
		   "# Personal stdlib:\n"
		   "# let string stdlib = \"stdlib\"\n";
}

std::string MainTemplate()
{
	return "void <- main()\n"
		   "{\n"
		   "    println(\"Hello, Weasel!\")\n"
		   "}\n";
}

int CompileProject(bool checkOnly, LogTarget logTarget)
{
	const auto projectDir = fs::current_path();
	const auto configFile = projectDir / ConfigFileName;
	if (!fs::exists(configFile))
	{
		std::cerr << "wkit: не найден " << ConfigFileName << " в текущем каталоге" << std::endl;
		return 1;
	}

	const WkitConfig config = WkitConfigLoader::Load(configFile);
	if (config.entry.empty())
	{
		std::cerr << "wkit: в конфигурации не задан entry" << std::endl;
		return 1;
	}

	const auto entry = projectDir / config.entry;
	if (!fs::exists(entry))
	{
		std::cerr << "wkit: файл entry не найден: " << entry.string() << std::endl;
		return 1;
	}

	const auto toolchainDir = GetExecutableDir();
	const auto grammar = toolchainDir / GrammarFileName;
	if (!fs::exists(grammar))
	{
		std::cerr << "wkit: не найден файл грамматики рядом с wkit: " << grammar.string() << std::endl;
		return 1;
	}

	const auto stdlibDir = config.stdlib.empty()
		? (toolchainDir / StdlibDirName)
		: (projectDir / config.stdlib);

	Logger::Init(LoggerFactory::Create(logTarget));

	auto context = LanguageContextBuilder::Build(grammar);

	CompilerOptions options;
	options.sourceFile = entry;
	options.grammarFile = grammar;
	options.stdlibDir = stdlibDir;
	options.checkOnly = checkOnly;
	options.logTarget = logTarget;

	const bool isSuccess = CompilerPipeline::Compile(options, context);
	return isSuccess ? 0 : 1;
}
} // namespace

int WkitCommands::New(const std::string& projectName)
{
	const auto root = fs::current_path() / projectName;
	if (fs::exists(root))
	{
		std::cerr << "wkit: каталог уже существует: " << root.string() << std::endl;
		return 1;
	}

	fs::create_directories(root / "src");
	WriteFile(root / ConfigFileName, ConfigTemplate(projectName));
	WriteFile(root / "src" / "main.wes", MainTemplate());

	std::cout << "wkit: создан проект " << projectName << std::endl;
	std::cout << "  cd " << projectName << " && wkit run" << std::endl;
	return 0;
}

int WkitCommands::Build()
{
	const int code = CompileProject(true, LogTarget::None);
	if (code == 0)
	{
		std::cout << "wkit: сборка успешна" << std::endl;
	}
	return code;
}

int WkitCommands::Run()
{
	return CompileProject(false, LogTarget::None);
}

int WkitCommands::Debug()
{
	return CompileProject(false, LogTarget::Console);
}

int WkitCommands::Format()
{
	const auto projectDir = fs::current_path();
	int formatted = 0;
	int failures = 0;

	for (const auto& entry : fs::recursive_directory_iterator(projectDir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".wes")
		{
			continue;
		}
		if (entry.path().filename() == ConfigFileName)
		{
			continue;
		}

		CompilerOptions options;
		options.sourceFile = entry.path();
		if (Formatter::FormatFile(options))
		{
			std::cout << "wkit: отформатирован " << fs::relative(entry.path(), projectDir).string() << std::endl;
			++formatted;
		}
		else
		{
			++failures;
		}
	}

	if (formatted == 0 && failures == 0)
	{
		std::cout << "wkit: файлы .wes не найдены" << std::endl;
	}
	return failures == 0 ? 0 : 1;
}
