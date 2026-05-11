#include "CommandLineParser.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>

namespace
{
struct FlagRule
{
	std::string_view name;
	bool requiresValue;
	std::function<void(CompilerOptions&, std::string_view)> apply;
};

void AssertIsEnoughArguments(int argc)
{
	if (argc < 2)
	{
		throw std::runtime_error("Проверь аргументы: wesc <name.wes> [-l | -lf] [-g <name.txt>] [-nasm]");
	}
}

void AssertIsFlagKnown(bool isKnown, std::string_view flagName)
{
	if (!isKnown)
	{
		throw std::runtime_error("Неизвестный флаг: " + std::string(flagName));
	}
}

void AssertIsFlagValuePresent(bool isPresent, std::string_view flagName)
{
	if (!isPresent)
	{
		throw std::runtime_error("Не указано значение для флага: " + std::string(flagName));
	}
}

void AssertIsSourceFileNotSet(bool isEmpty, std::string_view arg)
{
	if (!isEmpty)
	{
		throw std::runtime_error("Обнаружен лишний аргумент: " + std::string(arg));
	}
}

void AssertIsSourceFileSpecified(bool isSpecified)
{
	if (!isSpecified)
	{
		throw std::runtime_error("Не указан исходный файл для компиляции");
	}
}

bool IsFlag(std::string_view arg)
{
	return !arg.empty() && arg[0] == '-';
}

std::vector<FlagRule> GetFlagRules()
{
	return {
		{"-l", false, [](CompilerOptions& opt, std::string_view) {
			 opt.logTarget = LogTarget::Console;
		 }},
		{"-lf", false, [](CompilerOptions& opt, std::string_view) {
			 opt.logTarget = LogTarget::File;
		 }},
		{"-g", true, [](CompilerOptions& opt, std::string_view val) {
			 opt.grammarFile = val;
		 }},
		{"-nasm", false, [](CompilerOptions& opt, std::string_view) {
			 opt.emitNasm = true;
		 }}};
}
} // namespace

CompilerOptions CommandLineParser::Parse(int argc, char* argv[])
{
	AssertIsEnoughArguments(argc);

	CompilerOptions options;
	const std::vector<std::string_view> args(argv + 1, argv + argc);
	const std::vector<FlagRule> rules = GetFlagRules();

	for (size_t i = 0; i < args.size(); ++i)
	{
		const std::string_view currentArg = args[i];

		if (IsFlag(currentArg))
		{
			auto it = std::ranges::find_if(rules, [&](const FlagRule& rule) {
				return rule.name == currentArg;
			});

			AssertIsFlagKnown(it != rules.end(), currentArg);

			std::string_view value;
			if (it->requiresValue)
			{
				const bool hasValue = i + 1 < args.size() && !IsFlag(args[i + 1]);
				AssertIsFlagValuePresent(hasValue, currentArg);
				value = args[++i];
			}

			it->apply(options, value);
		}
		else
		{
			AssertIsSourceFileNotSet(options.sourceFile.empty(), currentArg);
			options.sourceFile = currentArg;
		}
	}

	AssertIsSourceFileSpecified(!options.sourceFile.empty());

	return options;
}