#pragma once

#include "src/logger/ILogger.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

class FileLogger final : public ILogger
{
public:
	explicit FileLogger(
		const std::filesystem::path& filePath = "log.txt",
		bool isEnabled = true);

	~FileLogger() override;

	void Log(const std::string& message) override;
	void SetEnabled(bool isEnabled) override;

private:
	std::atomic<bool> m_isEnabled;
	std::mutex m_mutex;
	std::ofstream m_file;
};