#pragma once

#include "types.h"

class Window
{
public:
	enum { kStringSize = 256 };

public:
    Window(HINSTANCE hInst, const wchar_t* name, HWND hParent);
    virtual ~Window();

public:
	bool Register();
	virtual void Create();
	void Show(int cmdShow=SW_SHOWDEFAULT);
	void SetTitle(const wchar_t* title);
	void SetClassName(const wchar_t* className);
	void SetFocus();

	void SetWindowProc(WNDPROC proc){ m_windowClass.lpfnWndProc = proc; }
	void SetWidth(int width){ m_width = width; }
	void SetHeight(int height){ m_height = height; }
	void SetX(int x){ m_x = x; }
	void SetY(int y){ m_y = y; }
	void SetWindowStyle(DWORD windowStyle){ m_windowStyle = windowStyle; }	
	void SetWindowStyleEx(DWORD windowStyleEx){ m_windowStyleEx = windowStyleEx; }
	void SetClassStyle(UINT style){ m_windowClass.style = style; }
	void SetMenuName(LPCWSTR menuName){ m_windowClass.lpszMenuName = menuName; }	
	void SetIcon(HICON icon){ m_windowClass.hIcon = icon; }
	void SetSmallIcon(HICON icon){ m_windowClass.hIconSm = icon; }
	void SetCursor(HCURSOR cursor){ m_windowClass.hCursor = cursor; }

	HWND GetHandle(){ return m_handle; }
	int GetWidth(){ return m_width; }
	int GetHeight(){ return m_height; }
	int GetX(){ return m_x; }
	int GetY(){ return m_y; }	

protected:
    HWND			m_handle;
    WNDCLASSEX		m_windowClass;
    DWORD			m_windowStyle;
    DWORD			m_windowStyleEx;
    wchar_t			m_title[kStringSize];
    wchar_t			m_className[kStringSize];
    int				m_x;
    int				m_y;
    int				m_width;
    int				m_height;
    HWND			m_parentHandle;
    HINSTANCE		m_instance;
};