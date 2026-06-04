#include "NativeRegistry.h"
#include "src/compiler/ast/ScalarTypeInfo.h"
#include "src/compiler/vm/memory/HeapObject.h"
#include <algorithm>
#include <iostream>
#include <string>

namespace
{
NativeFunction MakePrintUnsignedFunction()
{
	return NativeFunction{
		0, "print_unsigned", ScalarTypeInfo::Make(BaseType::Void), {{"value", ScalarTypeInfo::Make(BaseType::Uint)}}, [](std::span<const Value> args) -> Value {
			if (!args.empty()) std::cout << args[0].As<uint64_t>();
			return Value(0ull);
		}};
}

NativeFunction MakePrintSignedFunction()
{
	return NativeFunction{
		1, "print_signed", ScalarTypeInfo::Make(BaseType::Void), {{"value", ScalarTypeInfo::Make(BaseType::Int)}}, [](std::span<const Value> args) -> Value {
			if (!args.empty()) std::cout << args[0].As<int64_t>();
			return Value(0ull);
		}};
}

NativeFunction MakePrintRealFunction()
{
	return NativeFunction{
		2, "print_real", ScalarTypeInfo::Make(BaseType::Void), {{"value", ScalarTypeInfo::Make(BaseType::Real)}}, [](std::span<const Value> args) -> Value {
			if (!args.empty()) std::cout << args[0].As<double>();
			return Value(0ull);
		}};
}

NativeFunction MakePrintStringFunction()
{
	return NativeFunction{
		3, "print_string", ScalarTypeInfo::Make(BaseType::Void), {{"value", ScalarTypeInfo::Make(BaseType::String)}}, [](std::span<const Value> args) -> Value {
			if (!args.empty())
			{
				const auto* obj = reinterpret_cast<const HeapObject*>(args[0].AsRaw());
				if (obj)
				{
					const auto* strPtr = reinterpret_cast<const std::string*>(obj->GetField(0).AsRaw());
					if (strPtr) std::cout << *strPtr;
				}
			}
			return Value(0ull);
		}};
}

NativeFunction MakePrintBoolFunction()
{
	return NativeFunction{
		4, "print_bool", ScalarTypeInfo::Make(BaseType::Void), {{"value", ScalarTypeInfo::Make(BaseType::Bool)}}, [](std::span<const Value> args) -> Value {
			if (!args.empty()) std::cout << (args[0].As<uint32_t>() != 0 ? "true" : "false");
			return Value(0ull);
		}};
}

NativeFunction MakePrintSpaceFunction()
{
	return NativeFunction{
		5, "print_space", ScalarTypeInfo::Make(BaseType::Void), {}, [](std::span<const Value>) -> Value { std::cout << " "; return Value(0ull); }};
}

NativeFunction MakePrintNewlineFunction()
{
	return NativeFunction{
		6, "print_newline", ScalarTypeInfo::Make(BaseType::Void), {}, [](std::span<const Value>) -> Value { std::cout << std::endl; return Value(0ull); }};
}

void BuildCastRegistry(std::vector<NativeFunction>& registry)
{
	uint32_t nextId = static_cast<uint32_t>(registry.size());

	registry.push_back({nextId++, "__cast_int_real", nullptr, {}, [](std::span<const Value> args) { return Value(static_cast<double>(args[0].As<int64_t>())); }});
	registry.push_back({nextId++, "__cast_uint_real", nullptr, {}, [](std::span<const Value> args) { return Value(static_cast<double>(args[0].As<uint64_t>())); }});
	registry.push_back({nextId++, "__cast_real_int", nullptr, {}, [](std::span<const Value> args) { return Value(static_cast<int64_t>(args[0].As<double>())); }});
	registry.push_back({nextId++, "__cast_real_uint", nullptr, {}, [](std::span<const Value> args) { return Value(static_cast<uint64_t>(args[0].As<double>())); }});
	registry.push_back({nextId++, "__cast_int_uint", nullptr, {}, [](std::span<const Value> args) { int64_t val = args[0].As<int64_t>(); return Value(static_cast<uint64_t>(std::max(0LL, val))); }});
	registry.push_back({nextId++, "__cast_uint_int", nullptr, {}, [](std::span<const Value> args) { return Value(static_cast<int64_t>(args[0].As<uint64_t>())); }});
}

std::vector<NativeFunction> BuildRegistry()
{
	std::vector<NativeFunction> registry = {
		MakePrintUnsignedFunction(), MakePrintSignedFunction(), MakePrintRealFunction(), MakePrintStringFunction(), MakePrintBoolFunction(), MakePrintSpaceFunction(), MakePrintNewlineFunction()};
	BuildCastRegistry(registry);
	return registry;
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
	const auto it = std::ranges::find_if(all, [&name](const auto& fn) { return fn.name == name; });
	return it != all.end() ? &*it : nullptr;
}
} // namespace NativeRegistry