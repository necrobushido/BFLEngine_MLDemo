#include "WindowsDialog.h"

WindowsDialog::WindowsDialog(HINSTANCE hInst, WORD id, HWND hParent):
	m_id(id),
	m_handle(NULL), 
	m_instance(hInst), 
	m_x(CW_USEDEFAULT), 
	m_y(CW_USEDEFAULT),
	m_width(CW_USEDEFAULT), 
	m_height(CW_USEDEFAULT),
	m_parentHandle(hParent),
	m_windowProc(DefaultDialogProcedure)
{
}

WindowsDialog::~WindowsDialog()
{
	if( m_handle )
	{
		INT_PTR	result = 1;
		EndDialog(m_handle, result);
	}
}

void WindowsDialog::Create()
{
	m_handle = CreateDialogParam(m_instance, MAKEINTRESOURCE(m_id), m_parentHandle, m_windowProc, NULL);
	Assert(m_handle);

	RECT			ourRect;
	GetWindowRect(m_handle, &ourRect);

	m_width = ourRect.right - ourRect.left;
	m_height = ourRect.bottom - ourRect.top;
}

void WindowsDialog::Show(int cmdShow)
{
	ShowWindow(m_handle, cmdShow);
    UpdateWindow(m_handle);
}

void WindowsDialog::SetPos(int x, int y)
{
	SetWindowPos(m_handle, NULL, x, y, m_width, m_height, SWP_NOZORDER);
}

void WindowsDialog::EnableParentWindow(BOOL enabled)
{
	EnableWindow(m_parentHandle, enabled);
}

//	static
//typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
INT_PTR WindowsDialog::DefaultDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT	result = DefWindowProc(hwnd, msg, wParam, lParam);
	return result;
}