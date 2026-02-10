#include "Timer.h"
#include <chrono>

Timer::Timer()
	: m_startTime(Clock::now())
{
}

void Timer::Restart()
{
	m_startTime = Clock::now();
}

double Timer::Elapsed() const
{
	const auto endTime = Clock::now();
	const std::chrono::duration<double> duration = endTime - m_startTime;
	return duration.count();
}