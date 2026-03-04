#pragma once
#include <chrono>
#include <string>

class ScopedTimer
{
public:
	ScopedTimer(std::string phaseName, std::ostream& outputStream);
	~ScopedTimer();

	double Restart();
	[[nodiscard]] double Elapsed() const;

private:
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<Clock>;

	TimePoint m_startTime;
	std::string m_phaseName;
	std::ostream& m_outputStream;
};