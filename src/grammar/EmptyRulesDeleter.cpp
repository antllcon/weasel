#include "EmptyRulesDeleter.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace
{
void AssertIsSingleEpsilonSymbol(size_t count)
{
	if (count > 1)
	{
		throw std::runtime_error("Правило содержит более одного пустого символа для генерации");
	}
}

bool IsEpsilonAlternative(const raw::Alternative& alternative)
{
	return alternative.size() == 1 && alternative.front() == EMPTY_SYMBOL;
}

std::optional<std::string> FindNullableRuleName(const raw::Rules& rules)
{
	if (rules.size() < 2)
	{
		return std::nullopt;
	}

	for (size_t i = 1; i < rules.size(); ++i)
	{
		for (const auto& alternative : rules[i].alternatives)
		{
			if (IsEpsilonAlternative(alternative))
			{
				return rules[i].name;
			}
		}
	}

	return std::nullopt;
}

void PropagateNullableRule(raw::Rules& rules, const std::string& emptyRuleName)
{
	for (auto& rule : rules)
	{
		raw::Alternatives newAlternativesToAdd;

		// Фиксируем изначальный размер, чтобы не уйти в бесконечный цикл при добавлении новых правил
		const size_t initialSize = rule.alternatives.size();
		for (size_t i = 0; i < initialSize; ++i)
		{
			const auto& alternative = rule.alternatives[i];

			// Считаем, сколько раз пустой нетерминал встречается в текущей альтернативе
			const size_t emptyCount = std::ranges::count(alternative, emptyRuleName);

			if (emptyCount == 0)
			{
				continue;
			}

			// Генерируем 2^N - 1 комбинаций (маска 0 - это исходное правило, его не трогаем)
			const size_t combinationsCount = 1ull << emptyCount;
			for (size_t mask = 1; mask < combinationsCount; ++mask)
			{
				raw::Alternative newAlternative;
				size_t currentEmptyOccurrence = 0;

				for (const auto& symbol : alternative)
				{
					if (symbol == emptyRuleName)
					{
						// Проверяем бит в маске. Если он установлен (1), мы "удаляем" символ (не добавляем его)
						if ((mask & (1ull << currentEmptyOccurrence)) == 0)
						{
							newAlternative.push_back(symbol);
						}
						currentEmptyOccurrence++;
					}
					else
					{
						// Обычные символы и другие нетерминалы добавляем всегда
						newAlternative.push_back(symbol);
					}
				}

				// Если мы удалили вообще всё, заменяем на e (пустой символ)
				if (newAlternative.empty())
				{
					newAlternative.push_back(EMPTY_SYMBOL);
				}

				// Проверяем, нет ли уже такой альтернативы (чтобы избежать дубликатов, например при S -> A A)
				auto isDuplicate = [&](const raw::Alternative& alt) {
					return alt == newAlternative;
				};

				if (std::ranges::find_if(rule.alternatives, isDuplicate) == rule.alternatives.end() && std::ranges::find_if(newAlternativesToAdd, isDuplicate) == newAlternativesToAdd.end())
				{
					newAlternativesToAdd.push_back(std::move(newAlternative));
				}
			}
		}

		// Добавляем все новые уникальные комбинации в правило
		for (auto& newAlt : newAlternativesToAdd)
		{
			rule.alternatives.push_back(std::move(newAlt));
		}
	}
}

void DeleteEpsilonAlternative(raw::Rules& rules, const std::string& ruleName)
{
	const auto it = std::ranges::find_if(rules, [&ruleName](const raw::Rule& rule) {
		return rule.name == ruleName;
	});

	if (it != rules.end())
	{
		std::erase_if(it->alternatives, IsEpsilonAlternative);
	}
}
} // namespace

EmptyRulesDeleter::EmptyRulesDeleter(raw::Rules rules)
	: m_rules(std::move(rules))
{
}

raw::Rules EmptyRulesDeleter::DeleteEmptyRules()
{
	if (m_rules.size() < 2)
	{
		return m_rules;
	}

	while (const auto emptyRuleName = FindNullableRuleName(m_rules))
	{
		PropagateNullableRule(m_rules, emptyRuleName.value());
		DeleteEpsilonAlternative(m_rules, emptyRuleName.value());
	}

	return m_rules;
}