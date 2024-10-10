// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include <limits.h>
#include <windows.h>
#include <atomic>
#include <process.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define PATH_BUFFER_SIZE	1024	//	not MAX_PATH