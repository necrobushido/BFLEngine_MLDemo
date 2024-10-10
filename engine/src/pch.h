// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdio.h>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>
#include <process.h>


#define GLEW_STATIC
//#include "gl\glew.h"
#include "..\ThirdParty\glew\include\GL\glew.h"
#include <gl\gl.h>								// Header File For The OpenGL32 Library
#include <gl\glu.h>								// Header File For The GLu32 Library
//#include <gl\glaux.h>							// Header File For The GLaux Library


#endif //PCH_H
