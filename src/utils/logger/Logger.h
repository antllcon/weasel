#pragma once
#include "ILogger.h"
#include <memory>
#include <string>

namespace Logger
{
void Init(std::shared_ptr<ILogger> logger);
void Log(const std::string& message);
void SetEnabled(bool isEnabled);
bool IsInitialized() noexcept;
} // namespace Logger