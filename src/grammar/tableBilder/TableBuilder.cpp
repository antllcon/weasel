#include "TableBuilder.h"

#include <stdexcept>
#include <unordered_map>

namespace
{
void AssertRulesNotEmpty(const Rules& rules)
{
	if (rules.empty())
	{
		throw std::runtime_error("Грамматика пуста, невозможно построить таблицу");
	}
}

bool IsEmptySymbol(const std::string& s)
{
	return s == EMPTY_SYMBOL;
}
} // namespace

TableBuilder::TableBuilder(const Rules& rules)
	: m_rules(rules)
{
}

std::vector<ParseTableRow> TableBuilder::BuildDetailed() const
{
	AssertRulesNotEmpty(m_rules);

	std::vector<ParseTableRow> rows;
	rows.reserve(128);

	std::unordered_map<std::string, size_t> headerIndex;

	for (const auto& rule : m_rules)
	{
		ParseTableRow header;

		header.idx = rows.size();
		header.kind = ParseTableRow::Kind::NonterminalHeader;
		header.name = rule.name;

		header.groupPointer = SIZE_MAX;
		header.nextPointer = SIZE_MAX;

		header.ruleIndex = SIZE_MAX;
		header.altIndex = SIZE_MAX;

		rows.push_back(header);

		headerIndex[rule.name] = header.idx;
	}

	// ---------- ШАГ 2 ----------
	// строим RHS цепочки

	for (size_t r = 0; r < m_rules.size(); ++r)
	{
		const auto& rule = m_rules[r];

		size_t headerIdx = headerIndex[rule.name];
		size_t prevAlt = SIZE_MAX;

		for (size_t a = 0; a < rule.alternatives.size(); ++a)
		{
			const auto& alt = rule.alternatives[a];

			size_t firstRhs = SIZE_MAX;
			size_t prevRhs = SIZE_MAX;

			// ε правило
			if (alt.rule.size() == 1 && IsEmptySymbol(alt.rule[0]))
			{
				ParseTableRow row;

				row.idx = rows.size();
				row.kind = ParseTableRow::Kind::RhsEmpty;
				row.name = EMPTY_SYMBOL;

				row.groupPointer = SIZE_MAX;
				row.nextPointer = 0;

				row.ruleIndex = r;
				row.altIndex = a;
				row.guides = alt.guides;

				rows.push_back(row);

				firstRhs = row.idx;
			}
			else
			{
				for (size_t pos = 0; pos < alt.rule.size(); ++pos)
				{
					const auto& sym = alt.rule[pos];

					ParseTableRow row;

					row.idx = rows.size();
					row.ruleIndex = r;
					row.altIndex = a;

					row.nextPointer = SIZE_MAX;
					row.groupPointer = SIZE_MAX;

					if (IsTerm(sym))
					{
						row.kind = ParseTableRow::Kind::RhsTerminal;
						row.name = sym;
					}
					else
					{
						row.kind = ParseTableRow::Kind::RhsNonterminal;
						row.name = sym;

						auto it = headerIndex.find(sym);
						if (it == headerIndex.end())
						{
							throw std::runtime_error(
								"Нетерминал без определения: " + sym);
						}

						row.groupPointer = it->second;
					}

					if (firstRhs == SIZE_MAX)
						firstRhs = row.idx;

					if (prevRhs != SIZE_MAX)
						rows[prevRhs].nextPointer = row.idx;

					rows.push_back(row);

					prevRhs = row.idx;
				}

				rows[prevRhs].nextPointer = 0;
			}

			if (prevAlt == SIZE_MAX)
			{
				rows[headerIdx].groupPointer = firstRhs;
			}
			else
			{
				rows[prevAlt].nextPointer = firstRhs;
			}

			prevAlt = firstRhs;

			rows[firstRhs].guides = alt.guides;
		}
	}

	return rows;
}