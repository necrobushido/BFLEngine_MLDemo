#include "GLWindow.h"

namespace
{
	GLWindow::ResizeCallback	*s_resizeCallback = NULL;
}

GLWindow::GLWindow(HINSTANCE hInst, const wchar_t* name, HWND hParent):
	Window(hInst, name, hParent),
	m_hDeviceContext(NULL),
	m_hRenderingContext(NULL),
	m_bitsPerPixel(16)
{
	wcsncpy(m_className, L"OpenGL", kStringSize-1);
}

GLWindow::~GLWindow()
{
	wglMakeCurrent(0, 0);
    if( m_hDeviceContext && 
		m_hRenderingContext )
	{
		wglDeleteContext(m_hRenderingContext);
		ReleaseDC(m_handle, m_hDeviceContext);

		m_hDeviceContext = 0;
		m_hRenderingContext = 0;
	}
}

void GLWindow::Create()
{
	Window::Create();

	m_hDeviceContext = GetDC(m_handle);
	Assert(m_hDeviceContext);

    PIXELFORMATDESCRIPTOR	pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		(BYTE)m_bitsPerPixel,
		0, 
		0, 
		0, 
		0, 
		0, 
		0,
		0,
		0,
		0,
		0, 
		0, 
		0, 
		0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 
		0, 
		0
	};

	int	pixelFormat = ChoosePixelFormat(m_hDeviceContext, &pfd);
	Assert(pixelFormat != 0);
    BOOL	success = SetPixelFormat(m_hDeviceContext, pixelFormat, &pfd);
	Assert(success);

    m_hRenderingContext = wglCreateContext(m_hDeviceContext);
	Assert(m_hRenderingContext);
	success = wglMakeCurrent(m_hDeviceContext, m_hRenderingContext);
	Assert(success);

    ResizeScene(m_width, m_height);
}

void GLWindow::ResizeScene(s32 width, s32 height)
{
	if( width <= 0 || height <= 0 )
		return;

	m_width = width;
	m_height = height;

	glViewport( 0, 0, m_width, m_height );

	if( s_resizeCallback != NULL )
	{
		s_resizeCallback(width, height);
	}
}

void GLWindow::SwapBuffers()
{
	::SwapBuffers(m_hDeviceContext);
}

//	static
void GLWindow::SetResizeCallback(ResizeCallback *rscb)
{
	s_resizeCallback = rscb;
}