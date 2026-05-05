#pragma once
#include "src/grammar/lalr/LalrTypes.h"
#include <cstdint>
#include <vector>

namespace LalrTableSerializer
{
std::vector<uint8_t> Serialize(const LalrTable& table);
LalrTable Deserialize(const std::vector<uint8_t>& data);
} // namespace LalrTableSerializer