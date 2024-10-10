#pragma once

#include "types.h"

void ErrorDialog(const char *buffer);
void DebugPrint(const char *buffer);

inline void DoNotPanic()
{
}

inline void _Panic(const char *message)
{
#ifdef NDEBUG
	ErrorDialog(message);
	__debugbreak();
#else
	DebugPrint(message);
	__debugbreak();
	while(1){}
#endif
}

inline void _Panicf(const char *formatBuffer, ...)
{
	va_list	argumentList;
	va_start(argumentList, formatBuffer);

	const u16	kBufferSize = 260;
	char		buffer[kBufferSize];
	vsnprintf(buffer, kBufferSize, formatBuffer, argumentList);
	_Panic(buffer);
}

inline void _GLPanic(int error)
{
	if( error == GL_NO_ERROR )
		return;

	char	errorString[256];
	char	formatString[256];
	strcpy(formatString, "GL Error : %s\n");
	switch(error)
	{
	case GL_INVALID_ENUM:
		sprintf(errorString, formatString, "Invalid Enum");
		break;

	case GL_INVALID_VALUE:
		sprintf(errorString, formatString, "Invalid Value");
		break;

	case GL_INVALID_OPERATION:
		sprintf(errorString, formatString, "Invalid Operation");
		break;

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		sprintf(errorString, formatString, "Framebuffer Operation");
		break;

	case GL_OUT_OF_MEMORY:
		sprintf(errorString, formatString, "Out of Memory");
		break;

	case GL_STACK_UNDERFLOW:
		sprintf(errorString, formatString, "Stack Underflow");
		break;

	case GL_STACK_OVERFLOW:
		sprintf(errorString, formatString, "Stack Overflow");
		break;

	default:
		sprintf(errorString, formatString, "Unknown Error");
		break;
	}

	_Panic(errorString);
}

#define Assert(x)			((!(x)) ? _Panicf("Assertion failed, line = %d, file = %s\n", __LINE__, __FILE__) : DoNotPanic())
#define AssertMsg(x, ...)	((!(x)) ? _Panicf(__VA_ARGS__) : DoNotPanic())
#define AssertGL()			(_GLPanic(glGetError()))