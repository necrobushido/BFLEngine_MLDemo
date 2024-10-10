#include "WindowsFileDialog.h"

#include <Commdlg.h>

namespace
{
	enum{ kTempBufferSize = 256 };

	wchar_t			wFileNoPathOutputBuffer_Unused[kTempBufferSize];
}

void InitOFN(OPENFILENAME& openFileName, HWND dialogOwner, const wchar_t* fileFilter, const wchar_t* fileExtension, const wchar_t* initialDirectory, wchar_t* selectedFullPathOut)
{
	memset(&openFileName, 0, sizeof(OPENFILENAME));

	//	input
	openFileName.lStructSize = sizeof(OPENFILENAME);
	openFileName.hwndOwner = dialogOwner;
	openFileName.hInstance = 0;
	openFileName.lpstrFilter = fileFilter;				//	description of extension, null char, extension, null char, null char (implicit in string)
	openFileName.lpstrCustomFilter = NULL;
	openFileName.nMaxCustFilter = NULL;
	openFileName.nFilterIndex = NULL;
	openFileName.lpstrInitialDir = initialDirectory;
	openFileName.lpstrTitle = NULL;
	openFileName.Flags = OFN_FILEMUSTEXIST;					
	openFileName.lpstrDefExt = fileExtension;

	//	output
	selectedFullPathOut[0] = '\0';
	openFileName.lpstrFile = selectedFullPathOut;
	openFileName.nMaxFile = kTempBufferSize;

	openFileName.lpstrFileTitle = wFileNoPathOutputBuffer_Unused;
	openFileName.nMaxFileTitle = kTempBufferSize;
}

BOOL ShowSaveDialog(HWND dialogOwner, const wchar_t* fileFilter, const wchar_t* fileExtension, const wchar_t* initialDirectory, wchar_t* selectedFullPathOut)
{
	OPENFILENAME	openFileName;
	InitOFN(openFileName, dialogOwner, fileFilter, fileExtension, initialDirectory, selectedFullPathOut);
					
	return GetSaveFileName(&openFileName);
}

BOOL ShowLoadDialog(HWND dialogOwner, const wchar_t* fileFilter, const wchar_t* fileExtension, const wchar_t* initialDirectory, wchar_t* selectedFullPathOut)
{
	OPENFILENAME	openFileName;
	InitOFN(openFileName, dialogOwner, fileFilter, fileExtension, initialDirectory, selectedFullPathOut);
					
	return GetOpenFileName(&openFileName);
}