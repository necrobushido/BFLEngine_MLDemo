#pragma once

#include "types.h"

//	this was intended to facilitate using static dialogs as part of a menu
//	if you want a popup dialog, use the "DialogBox" Windows function
class WindowsDialog
{
public:
	WindowsDialog(HINSTANCE hInst, WORD id, HWND hParent);
	~WindowsDialog();

public:
	virtual void Create();
	void Show(int cmdShow=SW_SHOWDEFAULT);
	void SetPos(int x, int y);

	void SetWindowProc(DLGPROC proc){ m_windowProc = proc; }
	void SetWidth(int width){ m_width = width; }
	void SetHeight(int height){ m_height = height; }
	void SetX(int x){ m_x = x; }
	void SetY(int y){ m_y = y; }

	HWND GetHandle(){ return m_handle; }
	int GetWidth(){ return m_width; }
	int GetHeight(){ return m_height; }
	int GetX(){ return m_x; }
	int GetY(){ return m_y; }

	void EnableParentWindow(BOOL enabled);

protected:
	WORD			m_id;
	HWND			m_handle;
    int				m_x;
    int				m_y;
    int				m_width;
    int				m_height;
    HWND			m_parentHandle;
    HINSTANCE		m_instance;
	DLGPROC			m_windowProc;

public:
	static INT_PTR DefaultDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};