#pragma once
#include "src/compiler/ast/TypeInfo.h"
#include "src/compiler/stdlib/NativeTypes.h"
#include <memory>
#include <string>
#include <vector>

struct NativeFunction
{
	uint32_t id;
	std::string name;
	std::shared_ptr<TypeInfo> returnType;
	std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> params;
	NativeCallback callback;
};