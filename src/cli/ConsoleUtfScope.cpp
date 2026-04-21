#include "ConsoleUtfScope.h"
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

ConsoleUtf8Scope::ConsoleUtf8Scope()
	: m_previousOutputCp(GetConsoleOutputCP())
	, m_previousInputCp(GetConsoleCP())
{
	AssertIsEncodingSet(SetConsoleOutputCP(CP_UTF8) != 0);
	AssertIsEncodingSet(SetConsoleCP(CP_UTF8) != 0);
}

ConsoleUtf8Scope::~ConsoleUtf8Scope() noexcept
{
	SetConsoleOutputCP(m_previousOutputCp);
	SetConsoleCP(m_previousInputCp);
}

#else

ConsoleUtf8Scope::ConsoleUtf8Scope()
	: m_previousLocale(SaveCurrentLocale())
{
	AssertIsEncodingSet(std::setlocale(LC_ALL, "") != nullptr);
}

ConsoleUtf8Scope::~ConsoleUtf8Scope() noexcept
{
	std::setlocale(LC_ALL, m_previousLocale.c_str());
}

#endif
