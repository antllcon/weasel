#pragma once
#include "src/logger/ILogger.h"
#include <atomic>

class ConsoleLogger final : public ILogger
{
public:
	explicit ConsoleLogger(
		bool isEnabled = true,
		bool showThreadId = true);

	void Log(const std::string& message) override;
	void SetEnabled(bool isEnabled) override;

	void SetShowThreadId(bool showThreadId);

private:
	std::atomic<bool> m_isEnabled;
	std::atomic<bool> m_showThreadId;
};