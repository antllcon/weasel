#include "CommandLineParser.h"

#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
constexpr std::string_view FlagLogConsole = "-l";
constexpr std::string_view FlagLogFile = "-lf";
constexpr std::string_view FlagGrammar = "-g";

void AssertHasEnoughArguments(int argc)
{
	if (argc < 2)
	{
		throw std::runtime_error("Использование: wesc <файл.wes> [-l | -lf] [-g <файл.txt>]");
	}
}

void AssertSourceFileSpecified(const std::filesystem::path& sourceFile)
{
	if (sourceFile.empty())
	{
		throw std::runtime_error("Не указан исходный файл для компиляции");
	}
}

void AssertHasFlagValue(size_t index, size_t totalArgs, std::string_view flagName)
{
	if (index + 1 >= totalArgs)
	{
		throw std::runtime_error("Не указано значение для флага: " + std::string(flagName));
	}
}

bool IsFlag(std::string_view arg)
{
	return !arg.empty() && arg[0] == '-';
}
} // namespace

CompilerOptions CommandLineParser::Parse(int argc, char* argv[])
{
	AssertHasEnoughArguments(argc);

	CompilerOptions options;
	const std::vector<std::string_view> args(argv + 1, argv + argc);

	for (size_t i = 0; i < args.size(); ++i)
	{
		const std::string_view currentArg = args[i];

		if (currentArg == FlagLogConsole)
		{
			options.logTarget = LogTarget::Console;
		}
		else if (currentArg == FlagLogFile)
		{
			options.logTarget = LogTarget::File;
		}
		else if (currentArg == FlagGrammar)
		{
			AssertHasFlagValue(i, args.size(), FlagGrammar);
			options.grammarFile = args[++i];
		}
		else if (options.sourceFile.empty() && !IsFlag(currentArg))
		{
			options.sourceFile = currentArg;
		}
	}

	AssertSourceFileSpecified(options.sourceFile);

	return options;
}