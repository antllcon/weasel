#pragma once

#include <string>

class ILogger
{
public:
	virtual ~ILogger() = default;

	virtual void Log(const std::string& message) = 0;
	virtual void SetEnabled(bool isEnabled) = 0;
};