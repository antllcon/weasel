#include "cli/ConsoleUtfScope.h"
#include "grammar/LanguageContext/LanguageContextBuilder.h"
#include "src/cli/CommandLineParser.h"
#include "src/compiler/CompilerPipeline.h"
#include "src/logger/LoggerFactory.h"
#include <iostream>

int main(int argc, char* argv[])
{
	ConsoleUtf8Scope consoleScope;

	try
	{
		const auto options = CommandLineParser::Parse(argc, argv);
		const auto logger = LoggerFactory::Create(options.logTarget);

		const auto languageContext = LanguageContextBuilder::Build(options.grammarFile, logger);
		const auto success = CompilerPipeline::Compile(options.sourceFile, languageContext, logger);

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