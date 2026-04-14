#pragma once

#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <locale>
#endif

namespace
{
inline void AssertIsEncodingConfigured(bool isConfigured)
{
	if (!isConfigured)
	{
		throw std::runtime_error("Couldn't configure the encoding of the console");
	}
}
} // namespace

class ConsoleUtf8Scope final
{
public:
	ConsoleUtf8Scope()
	{
		Enable();
	}

	~ConsoleUtf8Scope()
	{
		Restore();
	}

	ConsoleUtf8Scope(const ConsoleUtf8Scope&) = delete;
	ConsoleUtf8Scope& operator=(const ConsoleUtf8Scope&) = delete;
	ConsoleUtf8Scope(ConsoleUtf8Scope&&) = delete;
	ConsoleUtf8Scope& operator=(ConsoleUtf8Scope&&) = delete;

private:
#ifdef _WIN32
	UINT m_previousOutputCp{0};
	UINT m_previousInputCp{0};

	void EnableWindowsOutput()
	{
		m_previousOutputCp = GetConsoleOutputCP();
		bool isOutSuccess = SetConsoleOutputCP(CP_UTF8) != 0;
		AssertIsEncodingConfigured(isOutSuccess);
	}

	void EnableWindowsInput()
	{
		m_previousInputCp = GetConsoleCP();
		bool isInSuccess = SetConsoleCP(CP_UTF8) != 0;
		AssertIsEncodingConfigured(isInSuccess);
	}

	void RestoreWindowsOutput() const noexcept
	{
		if (m_previousOutputCp != 0)
		{
			SetConsoleOutputCP(m_previousOutputCp);
		}
	}

	void RestoreWindowsInput() const noexcept
	{
		if (m_previousInputCp != 0)
		{
			SetConsoleCP(m_previousInputCp);
		}
	}

	void Enable()
	{
		EnableWindowsOutput();
		EnableWindowsInput();
	}

	void Restore() const noexcept
	{
		RestoreWindowsOutput();
		RestoreWindowsInput();
	}
#else
	std::locale m_previousLocale;

	void EnableUnixLocale()
	{
		try
		{
			m_previousLocale = std::locale::global(std::locale("en_US.UTF-8"));
		}
		catch (const std::exception&)
		{
			AssertIsEncodingConfigured(false);
		}
	}

	void RestoreUnixLocale() noexcept
	{
		try
		{
			std::locale::global(m_previousLocale);
		}
		catch (...)
		{
		}
	}

	void Enable()
	{
		EnableUnixLocale();
	}

	void Restore() noexcept
	{
		RestoreUnixLocale();
	}
#endif
};