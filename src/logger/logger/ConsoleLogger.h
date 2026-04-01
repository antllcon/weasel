#pragma once

#include "src/logger/ILogger.h"
#include <atomic>

class ConsoleLogger final : public ILogger
{
public:
	explicit ConsoleLogger(bool isEnabled = true);
	~ConsoleLogger() override = default;

	void Log(const std::string& message) override;
	void SetEnabled(bool isEnabled) override;

private:
	std::atomic<bool> m_isEnabled;
};