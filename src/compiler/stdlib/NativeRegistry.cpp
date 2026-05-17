#include "NativeRegistry.h"
#include <algorithm>
#include <iostream>

namespace
{
const std::vector<NativeFunction>& BuildRegistry()
{
	static const std::vector<NativeFunction> registry = []()
	{
		std::vector<NativeFunction> fns;
		fns.push_back(NativeFunction{
			.id         = 0,
			.name       = "print",
			.returnType = ScalarTypeInfo::Make(BaseType::Voided),
			.params     = {{"value", ScalarTypeInfo::Make(BaseType::Number)}},
			.callback   = [](std::span<const Value> args) -> Value
			{
				if (!args.empty())
				{
					std::cout << args[0].As<uint32_t>() << std::endl;
				}
				return Value(static_cast<uint32_t>(0));
			}
		});
		return fns;
	}();
	return registry;
}
}

namespace NativeRegistry
{
const std::vector<NativeFunction>& GetAll()
{
	return BuildRegistry();
}

const NativeFunction* FindByName(const std::string& name)
{
	const auto& all = GetAll();
	const auto it = std::ranges::find_if(all, [&name](const auto& fn) {
		return fn.name == name;
	});
	return it != all.end() ? &(*it) : nullptr;
}
}
