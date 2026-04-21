#pragma once

#include <string>

class ConsoleUtf8Scope final
{
public:
	ConsoleUtf8Scope();
	~ConsoleUtf8Scope() noexcept;

	ConsoleUtf8Scope(const ConsoleUtf8Scope&) = delete;
	ConsoleUtf8Scope& operator=(const ConsoleUtf8Scope&) = delete;
	ConsoleUtf8Scope(ConsoleUtf8Scope&&) = delete;
	ConsoleUtf8Scope& operator=(ConsoleUtf8Scope&&) = delete;

private:
#ifdef _WIN32
	unsigned int m_previousOutputCp;
	unsigned int m_previousInputCp;
#else
	std::string m_previousLocale;
#endif
};
