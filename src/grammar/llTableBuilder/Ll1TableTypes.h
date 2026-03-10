#pragma once
#include <optional>
#include <string>
#include <vector>

struct Ll1TableRow
{
	size_t index;
	std::string guideSet;
	bool shift;
	std::optional<size_t> transition;
	bool stack;
	bool error;
	bool finalState;
};

using Ll1Table = std::vector<Ll1TableRow>;