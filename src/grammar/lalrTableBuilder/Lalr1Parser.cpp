#include "Lalr1Parser.h"
#include <sstream>
#include <stdexcept>

namespace
{
void AssertIsTokensNotEmpty(bool isEmpty)
{
	if (isEmpty)
	{
		throw std::runtime_error("Входная строка пуста, разбор LALR(1) невозможен");
	}
}

void AssertIsActionFound(bool isFound, const std::string& token, size_t pos)
{
	if (!isFound)
	{
		throw std::runtime_error("Синтаксическая ошибка: непредвиденный токен '" + token + "' на позиции " + std::to_string(pos));
	}
}

void AssertIsGotoFound(bool isFound, const std::string& nonTerminal)
{
	if (!isFound)
	{
		throw std::runtime_error("Внутренняя ошибка таблиц: отсутствует переход (Goto) для '" + nonTerminal + "'");
	}
}

std::string FormatStateStack(const std::vector<size_t>& stack)
{
	std::ostringstream stream;
	for (size_t i = 0; i < stack.size(); ++i)
	{
		stream << stack[i];
		if (i + 1 < stack.size())
		{
			stream << " ";
		}
	}
	return stream.str();
}

std::string FormatSymbolStack(const std::vector<std::string>& stack)
{
	std::ostringstream stream;
	for (size_t i = 0; i < stack.size(); ++i)
	{
		stream << stack[i];
		if (i + 1 < stack.size())
		{
			stream << " ";
		}
	}
	return stream.str();
}

std::string FormatInput(const std::vector<std::string>& tokens, size_t pos)
{
	std::ostringstream stream;
	for (size_t i = pos; i < tokens.size(); ++i)
	{
		stream << tokens[i];
		if (i + 1 < tokens.size())
		{
			stream << " ";
		}
	}
	return stream.str();
}

size_t GetAlternativeLength(const raw::Alternative& alt)
{
	if (alt.size() == 1 && alt.front() == EMPTY_SYMBOL)
	{
		return 0;
	}
	return alt.size();
}
} // namespace

Lalr1Parser::Lalr1Parser(LalrTable table)
	: m_table(std::move(table))
{
}

std::vector<LalrParseStep> Lalr1Parser::Parse(const std::vector<std::string>& tokens) const
{
	AssertIsTokensNotEmpty(tokens.empty());

	std::vector<LalrParseStep> steps;
	std::vector<size_t> stateStack;
	std::vector<std::string> symbolStack;

	stateStack.push_back(0);
	size_t ip = 0;
	size_t stepCounter = 1;
	bool isAccepted = false;

	while (!isAccepted)
	{
		const std::string lookahead = ip < tokens.size() ? tokens[ip] : END_SYMBOL;
		const size_t currentState = stateStack.back();

		LalrParseStep step;
		step.stepNumber = stepCounter++;
		step.stateStack = FormatStateStack(stateStack);
		step.symbolStack = FormatSymbolStack(symbolStack);
		step.input = FormatInput(tokens, ip);

		const auto& actions = m_table.actionTable[currentState];
		const bool hasAction = actions.contains(lookahead);

		AssertIsActionFound(hasAction, lookahead, ip);

		const auto& [type, ruleIndex, altIndex] = actions.at(lookahead);

		if (type == LalrActionType::Shift)
		{
			step.action = "Shift " + std::to_string(ruleIndex);
			stateStack.push_back(ruleIndex);
			symbolStack.push_back(lookahead);
			ip++;
		}
		else if (type == LalrActionType::Reduce)
		{
			const raw::Rule& rule = m_table.augmentedRules[ruleIndex];
			const raw::Alternative& alt = rule.alternatives[altIndex];
			const size_t lengthToPop = GetAlternativeLength(alt);

			for (size_t i = 0; i < lengthToPop; ++i)
			{
				stateStack.pop_back();
				symbolStack.pop_back();
			}

			const size_t stateAfterPop = stateStack.back();
			const auto& gotos = m_table.gotoTable[stateAfterPop];

			AssertIsGotoFound(gotos.contains(rule.name), rule.name);

			const size_t nextState = gotos.at(rule.name);
			stateStack.push_back(nextState);
			symbolStack.push_back(rule.name);

			step.action = "Reduce " + rule.name + " -> ...";
		}
		else if (type == LalrActionType::Accept)
		{
			step.action = "Accept";
			isAccepted = true;
		}

		steps.push_back(std::move(step));
	}

	return steps;
}