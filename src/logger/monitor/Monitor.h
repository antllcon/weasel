#pragma once

#include "src/logger/ILogger.h"
#include <memory>
#include <thread>

class CpuUsageMonitor
{
public:
	explicit CpuUsageMonitor(
		std::shared_ptr<ILogger> logger,
		unsigned int intervalMs = 1000);

	~CpuUsageMonitor();

private:
	void MonitorLoop(const std::stop_token& stopToken) const;

	std::shared_ptr<ILogger> m_logger;
	unsigned int m_intervalMs;

	std::jthread m_worker;
};