#include "Monitor.h"

#include <sstream>
#include <stdexcept>
#include <thread>
#include <windows.h>

namespace
{
void AssertIsIntervalValid(const unsigned int interval)
{
	if (interval == 0)
	{
		throw std::runtime_error("Интервал мониторинга должен быть положительным");
	}
}

ULONGLONG FileTimeToUInt64(const FILETIME& ft)
{
	return static_cast<ULONGLONG>(ft.dwHighDateTime) << 32 | static_cast<ULONGLONG>(ft.dwLowDateTime);
}

double CalculateCpuUsage(
	const ULONGLONG idleDiff,
	const ULONGLONG kernelDiff,
	const ULONGLONG userDiff)
{
	const ULONGLONG total = kernelDiff + userDiff;

	if (total == 0)
	{
		return 0.0;
	}

	return (1.0 - static_cast<double>(idleDiff) / total) * 100.0;
}

std::string BuildBar(const double usage)
{
	constexpr int width = 16;

	const int filled = static_cast<int>(usage / 100.0 * width);

	std::string bar;

	for (int i = 0; i < width; ++i)
	{
		bar += i < filled ? '#' : ' ';
	}

	return bar;
}
} // namespace

CpuUsageMonitor::CpuUsageMonitor(
	std::shared_ptr<ILogger> logger,
	const unsigned int intervalMs)
	: m_logger(std::move(logger))
	, m_intervalMs(intervalMs)
{
	AssertIsIntervalValid(m_intervalMs);

	m_worker = std::jthread(
		[this](const std::stop_token& stopToken) {
			MonitorLoop(stopToken);
		});
}

CpuUsageMonitor::~CpuUsageMonitor()
{
}

void CpuUsageMonitor::MonitorLoop(const std::stop_token& stopToken) const
{
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	ULONGLONG prevIdle = FileTimeToUInt64(idleTime);
	ULONGLONG prevKernel = FileTimeToUInt64(kernelTime);
	ULONGLONG prevUser = FileTimeToUInt64(userTime);

	while (!stopToken.stop_requested())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalMs));

		GetSystemTimes(&idleTime, &kernelTime, &userTime);

		const auto idle = FileTimeToUInt64(idleTime);
		const auto kernel = FileTimeToUInt64(kernelTime);
		const auto user = FileTimeToUInt64(userTime);

		const auto usage = CalculateCpuUsage(
			idle - prevIdle,
			kernel - prevKernel,
			user - prevUser);

		prevIdle = idle;
		prevKernel = kernel;
		prevUser = user;

		std::ostringstream stream;

		stream << "CPU [" << BuildBar(usage) << "] "
			   << static_cast<int>(usage) << "%";

		m_logger->Log(stream.str());
	}
}