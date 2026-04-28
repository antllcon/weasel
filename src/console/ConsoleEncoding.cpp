#include "ConsoleEncoding.h"
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <clocale>
#endif

namespace
{
void AssertIsEncodingSet(bool success)
{
	if (!success)
	{
		throw std::runtime_error("Couldn't configure the encoding of the console");
	}
}

#ifndef _WIN32
std::string SaveCurrentLocale()
{
	const char* locale = std::setlocale(LC_ALL, nullptr);
	return locale ? locale : "";
}
#endif
}

#ifdef _WIN32

ConsoleEncoding::ConsoleEncoding()
	: m_previousOutputCp(GetConsoleOutputCP())
	, m_previousInputCp(GetConsoleCP())
{
	AssertIsEncodingSet(SetConsoleOutputCP(CP_UTF8) != 0);
	AssertIsEncodingSet(SetConsoleCP(CP_UTF8) != 0);
}

ConsoleEncoding::~ConsoleEncoding() noexcept
{
	SetConsoleOutputCP(m_previousOutputCp);
	SetConsoleCP(m_previousInputCp);
}

#else

ConsoleEncoding::ConsoleEncoding()
	: m_previousLocale(SaveCurrentLocale())
{
	AssertIsEncodingSet(std::setlocale(LC_ALL, "") != nullptr);
}

ConsoleEncoding::~ConsoleEncoding() noexcept
{
	std::setlocale(LC_ALL, m_previousLocale.c_str());
}

#endif
