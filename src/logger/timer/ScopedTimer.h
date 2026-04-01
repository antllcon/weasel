#pragma once
#include "src/logger/ILogger.h"
#include <chrono>
#include <memory>
#include <string>

class ScopedTimer
{
public:
	ScopedTimer(std::string phaseName, std::shared_ptr<ILogger> logger);
	~ScopedTimer();

	[[nodiscard]] double Elapsed() const;

private:
	using Clock = std::chrono::high_resolution_clock;

	std::string m_phaseName;
	std::shared_ptr<ILogger> m_logger;
	std::chrono::time_point<Clock> m_startTime;
};