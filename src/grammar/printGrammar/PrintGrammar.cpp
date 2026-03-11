#include "PrintGrammar.h"
#include <iostream>

namespace
{
bool IsFlagActive(OptimizationFlags value, OptimizationFlags flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}
} // namespace

raw::Rule PrintGrammar::MakeRule(const std::string& name, const raw::Alternatives& alts)
{
	return raw::Rule{name, alts};
}

void PrintGrammar::PrintRules(const raw::Rules& rules, const std::string& title)
{
	if (!title.empty())
	{
		std::cout << title << std::endl;
	}

	for (const auto& [name, alternatives] : rules)
	{
		std::cout << name << " -> ";
		for (size_t i = 0; i < alternatives.size(); ++i)
		{
			for (const auto& symbol : alternatives[i])
			{
				std::cout << symbol;
			}
			if (i + 1 < alternatives.size())
			{
				std::cout << " | ";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void PrintGrammar::PrintRulesWithGuides(const Rules& rules, const std::string& title)
{
	if (!title.empty())
	{
		std::cout << title << std::endl;
	}

	for (const auto& rule : rules)
	{
		std::cout << rule.name << " -> ";
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			const auto& alt = rule.alternatives[i];
			for (const auto& symbol : alt.rule)
			{
				std::cout << symbol;
			}

			std::cout << " { ";
			bool isFirst = true;
			for (const auto& term : alt.guides)
			{
				if (!isFirst)
				{
					std::cout << ", ";
				}
				std::cout << term;
				isFirst = false;
			}
			std::cout << " }";

			if (i + 1 < rule.alternatives.size())
			{
				std::cout << " | ";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void PrintGrammar::PrintAppliedOptimizations(OptimizationFlags flags)
{
	if (flags == OptimizationFlags::None)
	{
		std::cout << "  - Исходная грамматика уже является LL(1) (Без оптимизаций)" << std::endl;
		return;
	}

	if (IsFlagActive(flags, OptimizationFlags::LeftFactorize))
	{
		std::cout << "  - Левая факторизация" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::EliminateLeftRecursion))
	{
		std::cout << "  - Устранение левой рекурсии" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnreachable))
	{
		std::cout << "  - Удаление недостижимых правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnproductive))
	{
		std::cout << "  - Удаление непродуктивных правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteUnitRules))
	{
		std::cout << "  - Удаление цепных правил" << std::endl;
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteEmptyRules))
	{
		std::cout << "  - Удаление пустых правил" << std::endl;
	}

	std::cout << std::endl;
}