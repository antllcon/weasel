#pragma once
#include <string>

namespace WkitCommands
{
int New(const std::string& projectName);
int Build();
int Run();
int Debug();
int Format();
} // namespace WkitCommands
