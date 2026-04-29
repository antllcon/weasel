#include "grammar/LanguageContext/LanguageContextBuilder.h"
#include "logger/timer/ScopedTimer.h"
#include "src/compiler/CompilerPipeline.h"
#include "src/console/CommandLineParser.h"
#include "src/console/ConsoleEncoding.h"
#include "src/logger/LoggerFactory.h"
#include <iostream>

int main(int argc, char* argv[])
{
	ConsoleEncoding console;

	try
	{
		const auto options = CommandLineParser::Parse(argc, argv);
		const auto logger = LoggerFactory::Create(options.logTarget);

		ScopedTimer timer("Время компиляции", logger);
		const auto context = LanguageContextBuilder::Build(options.grammarFile, logger);
		const auto success = CompilerPipeline::Compile(options.sourceFile, context, logger);

		if (success)
		{
			std::cout << "[Compiler]\tУспешная компиляция" << std::endl;
			return EXIT_SUCCESS;
		}

		std::cout << "[Compiler]\tПроблемная компиляция" << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error]\t" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}