#pragma once

#include "types.h"

BOOL ShowSaveDialog(HWND dialogOwner, const wchar_t* fileFilter, const wchar_t* fileExtension, const wchar_t* initialDirectory, wchar_t* selectedFullPathOut);
BOOL ShowLoadDialog(HWND dialogOwner, const wchar_t* fileFilter, const wchar_t* fileExtension, const wchar_t* initialDirectory, wchar_t* selectedFullPathOut);