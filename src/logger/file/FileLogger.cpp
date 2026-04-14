#include "FileLogger.h"

#include <stdexcept>

namespace
{
void AssertFileIsOpen(const std::ofstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("Не удалось открыть файл для логирования: " + path.string());
	}
}
} // namespace

FileLogger::FileLogger(const std::filesystem::path& filePath, const bool isEnabled)
	: m_isEnabled(isEnabled)
{
	m_file.open(filePath, std::ios::out | std::ios::trunc);
	AssertFileIsOpen(m_file, filePath);
}

FileLogger::~FileLogger()
{
	std::lock_guard lock(m_mutex);
	if (m_file.is_open())
	{
		m_file.close();
	}
}

void FileLogger::Log(const std::string& message)
{
	if (!m_isEnabled.load(std::memory_order_relaxed))
	{
		return;
	}

	std::lock_guard lock(m_mutex);
	m_file << message << "\n";
	m_file.flush();
}

void FileLogger::SetEnabled(const bool isEnabled)
{
	m_isEnabled.store(isEnabled, std::memory_order_relaxed);
}