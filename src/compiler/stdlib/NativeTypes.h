#pragma once
#include "src/compiler/vm/value/Value.h"
#include <functional>
#include <span>

using NativeCallback = std::function<Value(std::span<const Value>)>;