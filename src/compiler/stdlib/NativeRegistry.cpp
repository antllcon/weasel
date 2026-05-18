#include "NativeRegistry.h"
#include <algorithm>
#include <iostream>

namespace
{
NativeFunction MakePrintFunction()
{
	return NativeFunction{
		.id = 0,
		.name = "print",
		.returnType = ScalarTypeInfo::Make(BaseType::Voided),
		.params = {{"value", ScalarTypeInfo::Make(BaseType::Number)}},
		.callback = [](std::span<const Value> args) -> Value {
			if (!args.empty())
			{
				std::cout << args[0].As<uint32_t>() << std::endl;
			}
			return Value(static_cast<uint32_t>(0));
		}};
}

std::vector<NativeFunction> BuildRegistry()
{
	return {MakePrintFunction()};
}

const std::vector<NativeFunction>& GetRegistryInstance() noexcept
{
	static const auto registry = BuildRegistry();
	return registry;
}
} // namespace

namespace NativeRegistry
{
const std::vector<NativeFunction>& GetAll() noexcept
{
	return GetRegistryInstance();
}

const NativeFunction* FindByName(const std::string& name) noexcept
{
	const auto& all = GetRegistryInstance();
	const auto it = std::ranges::find_if(all, [&name](const auto& fn) {
		return fn.name == name;
	});
	return it != all.end() ? &*it : nullptr;
}
} // namespace NativeRegistry