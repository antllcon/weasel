#include "compiler/pipline/CompilerPipeline.h"
#include "grammar/context/LanguageContextBuilder.h"
#include "utils/console/CommandLineParser.h"
#include "utils/console/ConsoleEncoding.h"
#include "utils/logger/LoggerFactory.h"
#include "utils/logger/timer/ScopedTimer.h"
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