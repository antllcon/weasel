#pragma once
#include "ILogger.h"
#include "console/ConsoleLogger.h"
#include "file/FileLogger.h"
#include "src/console/CommandLineParser.h"
#include <memory>

namespace LoggerFactory
{
inline std::shared_ptr<ILogger> Create(LogTarget target)
{
	switch (target)
	{
	case LogTarget::Console:
		return std::make_shared<ConsoleLogger>(true, false);
	case LogTarget::File:
		return std::make_shared<FileLogger>("log.txt", true);
	default:
		return nullptr;
	}
}
} // namespace LoggerFactory
