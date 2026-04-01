#include "ConsoleLogger.h"
#include <iostream>
#include <syncstream>

ConsoleLogger::ConsoleLogger(const bool isEnabled)
	: m_isEnabled(isEnabled)
{
}

void ConsoleLogger::Log(const std::string& message)
{
	if (!m_isEnabled.load(std::memory_order_relaxed))
	{
		return;
	}

	std::osyncstream(std::cout)
		<< "[thread " << std::this_thread::get_id() << "] "
		<< message
		<< std::endl;
}

void ConsoleLogger::SetEnabled(const bool isEnabled)
{
	m_isEnabled.store(isEnabled, std::memory_order_relaxed);
}