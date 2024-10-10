#pragma once

inline void ErrorDialog(const char *buffer)
{
	MessageBoxA(NULL, buffer, "ERROR", MB_OK);
}

inline void DebugPrint(const char *buffer)
{
	OutputDebugStringA(buffer);
}

inline void DebugPrintf(char *formatBuffer, ...)
{
	va_list	argumentList;
	va_start(argumentList, formatBuffer);

	const u16	kBufferSize = 260;
	char		buffer[kBufferSize];
	vsnprintf(buffer, kBufferSize, formatBuffer, argumentList);

	DebugPrint(buffer);
}

inline void DebugPrintf(const char *formatBuffer, ...)
{
	va_list	argumentList;
	va_start(argumentList, formatBuffer);

	const u16	kBufferSize = 260;
	char		buffer[kBufferSize];
	vsnprintf(buffer, kBufferSize, formatBuffer, argumentList);

	DebugPrint(buffer);
}