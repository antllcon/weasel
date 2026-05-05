#include "ScopedTimer.h"
#include "src/utils/logger/Logger.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
void AssertIsPhaseNameValid(const std::string& name)
{
	if (name.empty())
	{
		throw std::runtime_error("Имя фазы таймера не может быть пустым");
	}
}

std::string FormatMessage(const std::string& phase, const double elapsed)
{
	std::ostringstream stream;

	stream << "[Timer] \t"
		   << phase
		   << ": "
		   << std::fixed
		   << std::setprecision(6)
		   << elapsed
		   << " s";

	return stream.str();
}
} // namespace

ScopedTimer::ScopedTimer(std::string phaseName)
	: m_phaseName(std::move(phaseName))
	, m_startTime(Clock::now())
{
	AssertIsPhaseNameValid(m_phaseName);
}

ScopedTimer::~ScopedTimer()
{
	Logger::Log(FormatMessage(m_phaseName, Elapsed()));
}

double ScopedTimer::Elapsed() const
{
	const auto end = Clock::now();
	const std::chrono::duration<double> duration = end - m_startTime;
	return duration.count();
}