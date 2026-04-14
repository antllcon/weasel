#include "PrintGrammar.h"

#include <sstream>

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

void PrintGrammar::PrintRules(const raw::Rules& rules, const std::shared_ptr<ILogger>& logger, const std::string& title)
{
	if (!logger)
	{
		return;
	}

	std::ostringstream ss;

	if (!title.empty())
	{
		ss << title << "\n";
	}

	for (const auto& [name, alternatives] : rules)
	{
		ss << name << " -> ";
		for (size_t i = 0; i < alternatives.size(); ++i)
		{
			for (const auto& symbol : alternatives[i])
			{
				ss << symbol << " ";
			}
			if (i + 1 < alternatives.size())
			{
				ss << "| ";
			}
		}
		ss << "\n";
	}

	logger->Log(ss.str());
}

void PrintGrammar::PrintRulesWithGuides(const Rules& rules, const std::shared_ptr<ILogger>& logger, const std::string& title)
{
	if (!logger)
	{
		return;
	}

	std::ostringstream ss;

	if (!title.empty())
	{
		ss << title << "\n";
	}

	for (const auto& rule : rules)
	{
		ss << rule.name << " -> ";
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			const auto& alt = rule.alternatives[i];
			for (const auto& symbol : alt.rule)
			{
				ss << symbol;
			}

			ss << " { ";
			bool isFirst = true;
			for (const auto& term : alt.guides)
			{
				if (!isFirst)
				{
					ss << ", ";
				}
				ss << term;
				isFirst = false;
			}
			ss << " }";

			if (i + 1 < rule.alternatives.size())
			{
				ss << " | ";
			}
		}
		ss << "\n";
	}

	logger->Log(ss.str());
}

void PrintGrammar::PrintAppliedOptimizations(OptimizationFlags flags, const std::shared_ptr<ILogger>& logger)
{
	if (!logger)
	{
		return;
	}

	std::ostringstream ss;

	if (flags == OptimizationFlags::None)
	{
		ss << "  - Исходная грамматика уже является LL(1) (Без оптимизаций)\n";
		logger->Log(ss.str());
		return;
	}

	if (IsFlagActive(flags, OptimizationFlags::LeftFactorize))
	{
		ss << "  - Левая факторизация\n";
	}

	if (IsFlagActive(flags, OptimizationFlags::EliminateLeftRecursion))
	{
		ss << "  - Устранение левой рекурсии\n";
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnreachable))
	{
		ss << "  - Удаление недостижимых правил\n";
	}

	if (IsFlagActive(flags, OptimizationFlags::FilterUnproductive))
	{
		ss << "  - Удаление непродуктивных правил\n";
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteUnitRules))
	{
		ss << "  - Удаление цепных правил\n";
	}

	if (IsFlagActive(flags, OptimizationFlags::DeleteEmptyRules))
	{
		ss << "  - Удаление пустых правил\n";
	}

	logger->Log(ss.str());
}