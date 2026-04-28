#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

inline const std::string END_SYMBOL = "#";
inline const std::string EMPTY_SYMBOL = "e";

namespace raw
{
using Alternative = std::vector<std::string>;
using Alternatives = std::vector<Alternative>;

struct Rule
{
	std::string name;
	Alternatives alternatives;
};

using Rules = std::vector<Rule>;
}

inline bool IsTerm(std::string_view term)
{
	if (term.empty())
	{
		return false;
	}

	if (std::isupper(static_cast<unsigned char>(term[0])))
	{
		return false;
	}

	return true;
}
