#include "LlValidator.h"
#include <stdexcept>

namespace
{
void AssertIsNotEmpty(const Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика пуста, валидация LL(1) невозможна");
	}
}

bool HasIntersection(const Guides& lhs, const Guides& rhs)
{
	for (const auto& term : lhs)
	{
		if (rhs.contains(term))
		{
			return true;
		}
	}
	return false;
}
}

LlValidator::LlValidator(Rules rules)
	: m_rules(std::move(rules))
{
}

bool LlValidator::IsValid() const
{
	AssertIsNotEmpty(m_rules);

	for (const auto& rule : m_rules)
	{
		for (size_t i = 0; i < rule.alternatives.size(); ++i)
		{
			for (size_t j = i + 1; j < rule.alternatives.size(); ++j)
			{
				if (HasIntersection(rule.alternatives[i].guides, rule.alternatives[j].guides))
				{
					return false;
				}
			}
		}
	}

	return true;
}