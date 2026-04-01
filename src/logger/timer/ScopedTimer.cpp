#include "ScopedTimer.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace
{
void AssertIsPhaseNameValid(const std::string& name)
{
	if (name.empty())
	{
		throw std::runtime_error("Имя фазы таймера не может быть пустым");
	}
}

std::string FormatMessage(
	const std::string& phase,
	const double elapsed)
{
	std::ostringstream stream;

	stream << "[Timer] "
		   << phase
		   << ": "
		   << std::fixed
		   << std::setprecision(6)
		   << elapsed
		   << " s";

	return stream.str();
}
} // namespace

ScopedTimer::ScopedTimer(
	std::string phaseName,
	std::shared_ptr<ILogger> logger)
	: m_phaseName(std::move(phaseName))
	, m_logger(std::move(logger))
	, m_startTime(Clock::now())
{
	AssertIsPhaseNameValid(m_phaseName);
}

ScopedTimer::~ScopedTimer()
{
	const double elapsed = Elapsed();

	if (m_logger)
	{
		m_logger->Log(FormatMessage(m_phaseName, elapsed));
	}
}

double ScopedTimer::Elapsed() const
{
	const auto end = Clock::now();
	const std::chrono::duration<double> duration = end - m_startTime;
	return duration.count();
}