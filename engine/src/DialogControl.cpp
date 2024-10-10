#include "DialogControl.h"

DialogControl::DialogControl():
	m_parentDialogHandle(NULL),
	m_controlID(-1)
{
}

DialogControl::DialogControl(HWND parentDialogHandle, int controlID):
	m_parentDialogHandle(parentDialogHandle),
	m_controlID(controlID)
{
}

void DialogControl::Init(HWND parentDialogHandle, int controlID)
{
	m_parentDialogHandle = parentDialogHandle;
	m_controlID = controlID;
}