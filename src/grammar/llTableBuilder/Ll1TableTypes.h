#pragma once
#include <optional>
#include <string>
#include <vector>

struct Ll1TableRow
{
	size_t index;
	std::string name;
	std::string guides;
	bool error;
	std::optional<size_t> transition;
	bool shift;
	bool stack;
};

using Ll1Table = std::vector<Ll1TableRow>;