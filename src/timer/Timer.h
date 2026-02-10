#pragma once

#include <chrono>

class Timer
{
public:
	Timer();
	void Restart();
	[[nodiscard]] double Elapsed() const;

private:
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<Clock>;

	TimePoint m_startTime;
};