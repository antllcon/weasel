#pragma once
#include "src/compiler/stdlib/NativeFunction.h"
#include <string>
#include <vector>

namespace NativeRegistry
{
[[nodiscard]] const std::vector<NativeFunction>& GetAll() noexcept;
[[nodiscard]] const NativeFunction* FindByName(const std::string& name) noexcept;
}