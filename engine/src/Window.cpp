#include "Window.h"

Window::Window(HINSTANCE hInst, const wchar_t* name, HWND hParent): 
	m_handle(NULL), 
	m_instance(hInst), 
	m_windowStyle(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN),
	m_windowStyleEx(WS_EX_CLIENTEDGE), 
	m_x(CW_USEDEFAULT), 
	m_y(CW_USEDEFAULT),
	m_width(CW_USEDEFAULT), 
	m_height(CW_USEDEFAULT),
	m_parentHandle(hParent)
{
	wcsncpy(m_title, name, kStringSize-1);
	wcsncpy(m_className, name, kStringSize-1);

	m_windowClass.cbSize        = sizeof(WNDCLASSEX);
    m_windowClass.style         = 0; 
    m_windowClass.lpfnWndProc   = DefWindowProc;
    m_windowClass.cbClsExtra    = 0;
    m_windowClass.cbWndExtra    = 0;
    m_windowClass.hInstance     = m_instance;
    m_windowClass.hIcon         = LoadIcon(0, IDI_APPLICATION);
    m_windowClass.hCursor       = LoadCursor(0, IDC_ARROW);
    m_windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	//	was NULL
    m_windowClass.lpszMenuName  = NULL;
    m_windowClass.lpszClassName = m_className;
    m_windowClass.hIconSm       = LoadIcon(0, IDI_APPLICATION);
}

Window::~Window()
{
    UnregisterClass(m_className, m_instance);
}

bool Window::Register()
{
	return RegisterClassEx(&m_windowClass) != 0;
}

void _PanicTest(const char *message)
{
	DebugPrint(message);
	__debugbreak();
	while(1){}
}

void _PanicfTest(const char *formatBuffer, ...)
{
	va_list	argumentList;
	va_start(argumentList, formatBuffer);

	const u16	kBufferSize = 260;
	char		buffer[kBufferSize];
	vsnprintf(buffer, kBufferSize, formatBuffer, argumentList);
	_PanicTest(buffer);

	va_end(argumentList);
}

void Window::Create()
{
	m_handle = CreateWindowEx(	m_windowStyleEx,
								m_className,
								m_title,
								m_windowStyle,
								m_x,
								m_y,
								m_width,
								m_height,
								m_parentHandle,
								NULL,
								m_instance,
								NULL);


	Assert(m_handle);

	RECT	ourRect;
	GetWindowRect(m_handle, &ourRect);

	m_x = ourRect.left;
	m_y = ourRect.top;
}

void Window::Show(int cmdShow)
{
	ShowWindow(m_handle, cmdShow);
    UpdateWindow(m_handle);
}

void Window::SetTitle(const wchar_t* title)
{
	wcsncpy(m_title, title, kStringSize-1);
}

void Window::SetClassName(const wchar_t* className)
{
	wcsncpy(m_className, className, kStringSize-1);
}

void Window::SetFocus()
{
	::SetFocus(m_handle);
}