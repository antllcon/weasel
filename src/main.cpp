#include "logger/console/ConsoleLogger.h"
#include "logger/file/FileLogger.h"
#include "src/cli/CommandLineParser.h"
#include "src/compiler/CompilerPipeline.h"
#include <iostream>
#include <memory>
#include <windows.h>

namespace
{
std::shared_ptr<ILogger> CreateLogger(LogTarget target)
{
	switch (target)
	{
	case LogTarget::Console:
		return std::make_shared<ConsoleLogger>(true, false);
	case LogTarget::File:
		return std::make_shared<FileLogger>("log.txt", true);
	default:
		return nullptr;
	}
}
} // namespace

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	try
	{
		const auto options = CommandLineParser::Parse(argc, argv);
		auto logger = CreateLogger(options.logTarget);

		CompilerPipeline pipeline;
		pipeline.InitGrammar(options.grammarFile, logger);

		const auto success = pipeline.Compile(options.sourceFile, logger);

		if (success)
		{
			if (logger) logger->Log("\n[Compiler]\tКонец компиляции");
			return EXIT_SUCCESS;
		}

		if (logger) logger->Log("\n[Compiler]\tЕсть ошибки на этапе компиляции");
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error] " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}