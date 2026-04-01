#pragma once

#include <Windows.h>

class ConsoleEncoding
{
public:
	ConsoleEncoding();
	~ConsoleEncoding();

	ConsoleEncoding(const ConsoleEncoding&) = delete;
	ConsoleEncoding& operator=(const ConsoleEncoding&) = delete;

private:
	UINT m_oldOutputCodePage;
	UINT m_oldInputCodePage;
};