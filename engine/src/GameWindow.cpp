#include "GameWindow.h"

#include "Input.h"

namespace
{
	GameWindow	*g_GameWindowPtr = NULL;
}

GameWindow::GameWindow(HINSTANCE hInst, const wchar_t* name, HWND hParent):
	GLWindow(hInst, name, hParent),
	m_active(false)
{
	Assert(g_GameWindowPtr == NULL);
	g_GameWindowPtr = this;

	m_windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	m_windowClass.lpfnWndProc = GameWindow::WinProc;
}

GameWindow::~GameWindow()
{
	g_GameWindowPtr = NULL;
}

bool GameWindow::ShouldExit()
{
	return Input::KeyPressed(VK_ESCAPE);
}

//	static
LRESULT CALLBACK GameWindow::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch( uMsg )
	{
	case WM_ACTIVATE:
		if( !HIWORD( wParam ) )					// Check Minimization State
		{
			g_GameWindowPtr->m_active = TRUE;
			//SetCapture( hWnd );
		}
		else
		{
			g_GameWindowPtr->m_active = FALSE;
			//ReleaseCapture();
		}		
		return 0;

	case WM_SYSCOMMAND:
		switch( wParam )
		{
		case SC_SCREENSAVE:						//	Screensaver Trying To Start?
		case SC_MONITORPOWER:					//	Monitor Trying To Enter Powersave?
			return 0;							//	Prevent From Happening
		}
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		if( !Input::KeyHeld((u8)wParam) )
		{
			Input::ToggleKey((u8)wParam, true);
		}
		return 0;

	case WM_KEYUP:
		Input::ToggleKey((u8)wParam, false);
		return 0;

	case WM_CHAR:
		Input::AddText((u8)wParam);
		return 0;

	case WM_SIZE:
		g_GameWindowPtr->ResizeScene( LOWORD(lParam), HIWORD(lParam));  //	LoWord=Width, HiWord=Height
		return 0;

	case WM_COMMAND:
		switch(HIWORD(wParam)) 
		{
		case EN_SETFOCUS:
			{
				RECT		windowRect;
				GetWindowRect(hWnd, &windowRect);
				SetCursorPos( windowRect.right / 2, windowRect.bottom / 2 );
			}
			//ShowCursor( FALSE );
			break;

		case EN_KILLFOCUS:
			//ShowCursor( TRUE );
			break;

		default:
			break;
		}
		return 0;	

	case WM_LBUTTONDOWN:
		{
			int	xMouse = LOWORD(lParam);
			int	yMouse = HIWORD(lParam);
			Input::ToggleLeftMouse(true, xMouse, yMouse);
		}
		break;

	case WM_LBUTTONUP:
		{
			int	xMouse = LOWORD(lParam);
			int	yMouse = HIWORD(lParam);
			Input::ToggleLeftMouse(false, xMouse, yMouse);
		}
		break;

	case WM_RBUTTONDOWN:
		{
			int	xMouse = LOWORD(lParam);
			int	yMouse = HIWORD(lParam);
			Input::ToggleRightMouse(true, xMouse, yMouse);
		}
		break;

	case WM_RBUTTONUP:
		{
			int	xMouse = LOWORD(lParam);
			int	yMouse = HIWORD(lParam);
			Input::ToggleRightMouse(false, xMouse, yMouse);
		}
		break;

	case WM_MOUSEHOVER:
		break;

	case WM_MOUSEMOVE:
		{
			int	xMouse = LOWORD(lParam);
			int	yMouse = HIWORD(lParam);
			Input::SetMousePos(xMouse, yMouse);
		}
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam);
}