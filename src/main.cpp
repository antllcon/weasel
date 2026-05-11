#include "compiler/pipline/CompilerPipeline.h"
#include "grammar/context/LanguageContextBuilder.h"
#include "utils/console/CommandLineParser.h"
#include "utils/console/ConsoleEncoding.h"
#include "utils/logger/Logger.h"
#include "utils/logger/LoggerFactory.h"
#include "utils/logger/timer/ScopedTimer.h"
#include <iostream>

int main(int argc, char* argv[])
{
	ConsoleEncoding console;

	try
	{
		const auto options = CommandLineParser::Parse(argc, argv);
		Logger::Init(LoggerFactory::Create(options.logTarget));

		const auto context = LanguageContextBuilder::Build(options.grammarFile);

		ScopedTimer timer("Время компиляции");
		const bool success = CompilerPipeline::Compile(options, context);

		const auto message = success
			? "[Compiler]\tУспешная компиляция"
			: "[Compiler]\tПроблемная компиляция";
		std::cout << message << std::endl;

		return success ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[Error]\t" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}