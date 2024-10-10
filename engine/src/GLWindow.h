#pragma once

#include "Window.h"

class GLWindow : public Window
{
public:
	typedef void ResizeCallback(s32, s32);

public:
	GLWindow(HINSTANCE hInst, const wchar_t* name, HWND hParent);
	virtual ~GLWindow();

public:
	virtual void Create();
	void ResizeScene(s32 width, s32 height);
	void SetBitsPerPixel(int bpp){ m_bitsPerPixel = bpp; }
	void SwapBuffers();

protected:
	HDC		m_hDeviceContext;
	HGLRC	m_hRenderingContext;
	int		m_bitsPerPixel;

public:
	static void SetResizeCallback(ResizeCallback *rscb);
};

