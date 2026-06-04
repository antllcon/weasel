#pragma once
#include "src/compiler/ast/TypeInfo.h"
#include <string>

namespace PrintResolver
{
[[nodiscard]] std::string Resolve(const TypeInfo* typeInfo) noexcept;
}