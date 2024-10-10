#pragma once

#include "GLWindow.h"

class GameWindow : public GLWindow
{
public:
	GameWindow(HINSTANCE hInst, const wchar_t* name, HWND hParent);
	virtual ~GameWindow();

public:
	bool GetActive(){ return m_active; }
	bool ShouldExit();

protected:
	bool	m_active;

public:
	static LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

