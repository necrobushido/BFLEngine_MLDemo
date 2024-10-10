#include "EditControl.h"

void EditControl::GetText(char* buffer, int bufferSize)
{
	*((WORD*)&buffer[0]) = bufferSize;

	LRESULT	retValue = SendDlgItemMessage(m_parentDialogHandle, m_controlID, EM_GETLINE, 0, (LPARAM)buffer);

	buffer[retValue] = '\0';
}

void EditControl::SetText(const char* buffer)
{
	LRESULT	retValue = SendDlgItemMessage(m_parentDialogHandle, m_controlID, WM_SETTEXT, 0, (LPARAM)buffer);
}