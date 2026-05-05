#include "Logger.h"

namespace
{
std::shared_ptr<ILogger> g_logger;
} // namespace

namespace Logger
{
void Init(std::shared_ptr<ILogger> logger)
{
	g_logger = std::move(logger);
}

bool IsInitialized() noexcept
{
	return g_logger != nullptr;
}

void Log(const std::string& message)
{
	if (g_logger)
	{
		g_logger->Log(message);
	}
}

void SetEnabled(bool isEnabled)
{
	if (g_logger)
	{
		g_logger->SetEnabled(isEnabled);
	}
}
} // namespace Logger