#pragma once

#include "types.h"

class DialogControl
{
public:
	DialogControl();
	DialogControl(HWND parentDialogHandle, int controlID);

public:
	void Init(HWND parentDialogHandle, int controlID);
	int GetControlID(){ return m_controlID; }

protected:
	HWND	m_parentDialogHandle;
	int		m_controlID;
};