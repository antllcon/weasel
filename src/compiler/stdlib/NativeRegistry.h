#pragma once
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/vm/value/Value.h"
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

using NativeCallback = std::function<Value(std::span<const Value>)>;

struct NativeFunction
{
	uint32_t id;
	std::string name;
	std::shared_ptr<TypeInfo> returnType;
	std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> params;
	NativeCallback callback;
};

namespace NativeRegistry
{
const std::vector<NativeFunction>& GetAll() noexcept;
const NativeFunction* FindByName(const std::string& name) noexcept;
} // namespace NativeRegistry
