#include "libs/ConsoleUtfScope.h"
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
	ConsoleUtf8Scope consoleScope;

	try
	{
		const auto [sourceFile, grammarFile, logTarget] = CommandLineParser::Parse(argc, argv);

		const auto logger = CreateLogger(logTarget);

		CompilerPipeline pipeline;
		pipeline.InitGrammar(grammarFile, logger);
		const auto success = pipeline.Compile(sourceFile, logger);

		if (success)
		{
			if (logger) logger->Log("[Compiler]\tУспешная компиляция");
			return EXIT_SUCCESS;
		}

		if (logger) logger->Log("[Compiler]\tНе успешная компиляция");
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error]\t" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}