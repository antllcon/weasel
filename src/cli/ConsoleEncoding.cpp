#include "../console/ConsoleEncoding.h"
#include <stdexcept>

namespace
{
void AssertIsOsApiSuccessful(const BOOL result)
{
	if (!result)
	{
		throw std::runtime_error("Error call API OS");
	}
}

UINT GetOutputCodePage()
{
	return GetConsoleOutputCP();
}

UINT GetInputCodePage()
{
	return GetConsoleCP();
}

void SetUtf8Output()
{
	AssertIsOsApiSuccessful(SetConsoleOutputCP(CP_UTF8));
}

void SetUtf8Input()
{
	AssertIsOsApiSuccessful(SetConsoleCP(CP_UTF8));
}

void RestoreOutput(UINT cp)
{
	SetConsoleOutputCP(cp);
}

void RestoreInput(UINT cp)
{
	SetConsoleCP(cp);
}
}

ConsoleEncoding::ConsoleEncoding()
	: m_oldOutputCodePage(GetOutputCodePage())
	, m_oldInputCodePage(GetInputCodePage())
{
	SetUtf8Output();
	SetUtf8Input();
}

ConsoleEncoding::~ConsoleEncoding()
{
	RestoreOutput(m_oldOutputCodePage);
	RestoreInput(m_oldInputCodePage);
}