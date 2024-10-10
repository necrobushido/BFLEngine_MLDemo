#pragma once

#include "pch.h"
#include "dataTypes.h"

//#define ERROR(msg)			(MessageBox( NULL, msg, "Error!", MB_OK | MB_ICONINFORMATION ))
//#define QUERY(msg)			(MessageBox( NULL, msg, "Query?", MB_YESNO | MB_ICONEXCLAMATION ) == IDYES)

#include "EngineAssert.h"
#include "Print.h"

// A macro to determine the size of an array at compile time, so the size
// doesn't have to be maintained separately.
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

// Defining this constant enables module/line associations with every allocation.
// Adds 12 bytes per allocation, but does not slow anything down.
//#define MEMDEBUG

// Needs to be included by anyone that calls new / delete in order
// for MEMDEBUG to work on it.
//#include "EngineMemory.h"

// Endian swap
inline void SwapEndian(u16 *x)
{
	*x = (u16)((*x << 8) | (*x >> 8));
}

inline void SwapEndian(s16 *x)
{
	*x = s16((u16(*x) << 8) | (u16(*x) >> 8));
}

inline void SwapEndian(u32 *x)
{
	*x = (*x << 24) | ((*x << 8) & 0x00ff0000UL) | ((*x >> 8) & 0x0000ff00UL) | (*x >> 24);
}

inline void SwapEndian(s32 *x)
{
	*x = s32((u32(*x) << 24) | ((u32(*x) << 8) & 0x00ff0000UL) | ((u32(*x) >> 8) & 0x0000ff00UL) | (u32(*x) >> 24));
}