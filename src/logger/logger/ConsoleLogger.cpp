#include "ConsoleLogger.h"
#include <iostream>
#include <syncstream>
#include <thread>

ConsoleLogger::ConsoleLogger(
	const bool isEnabled,
	const bool showThreadId)
	: m_isEnabled(isEnabled)
	, m_showThreadId(showThreadId)
{
}

void ConsoleLogger::Log(const std::string& message)
{
	if (!m_isEnabled.load(std::memory_order_relaxed))
	{
		return;
	}

	std::osyncstream stream(std::cout);

	if (m_showThreadId.load(std::memory_order_relaxed))
	{
		stream << "[thread " << std::this_thread::get_id() << "]\t";
	}

	stream << message << std::endl;
}

void ConsoleLogger::SetEnabled(const bool isEnabled)
{
	m_isEnabled.store(isEnabled, std::memory_order_relaxed);
}

void ConsoleLogger::SetShowThreadId(const bool showThreadId)
{
	m_showThreadId.store(showThreadId, std::memory_order_relaxed);
}