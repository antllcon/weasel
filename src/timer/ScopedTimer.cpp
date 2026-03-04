#include "ScopedTimer.h"
#include <iomanip>
#include <stdexcept>
#include <utility>

namespace
{
void AssertIsPhaseNameValid(const std::string& name)
{
	if (name.empty())
	{
		throw std::invalid_argument("Имя фазы для таймера не может быть пустым");
	}
}
}

ScopedTimer::ScopedTimer(std::string phaseName, std::ostream& outputStream)
	: m_startTime(Clock::now())
	, m_phaseName(std::move(phaseName))
	, m_outputStream(outputStream)
{
	AssertIsPhaseNameValid(m_phaseName);
}

ScopedTimer::~ScopedTimer()
{
	m_outputStream << "[Timer] " << m_phaseName << ": "
				   << std::fixed << std::setprecision(3) << Elapsed()
				   << " s" << std::endl;
}

double ScopedTimer::Restart()
{
	const double elapsed = Elapsed();
	m_startTime = Clock::now();
	return elapsed;
}

double ScopedTimer::Elapsed() const
{
	const auto endTime = Clock::now();
	const std::chrono::duration<double> duration = endTime - m_startTime;
	return duration.count();
}