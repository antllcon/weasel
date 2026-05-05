#pragma once
#include <chrono>
#include <string>

class ScopedTimer
{
public:
	explicit ScopedTimer(std::string phaseName);
	~ScopedTimer();

	[[nodiscard]] double Elapsed() const;

private:
	using Clock = std::chrono::high_resolution_clock;

	std::string m_phaseName;
	std::chrono::time_point<Clock> m_startTime;
};