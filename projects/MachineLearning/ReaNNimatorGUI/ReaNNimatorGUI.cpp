#include "framework.h"
#include "ReaNNimatorGUI.h"

#include "Window.h"
#include "GLWindow.h"
#include "WindowsFileDialog.h"
#include "Clock.h"
#include "FileManager.h"
#include "Renderer.h"
#include "MainMenuDialog.h"

#include "PreviewScene.h"
#include "AnimGenBoneDictionary.h"

#define MAX_LOADSTRING 100

namespace
{
	HINSTANCE	hInst;								// current instance
	WCHAR		titleString[MAX_LOADSTRING];		// The title bar text
	WCHAR		classString[MAX_LOADSTRING];		// the main window class name

	enum
	{
		WIDTH		= 1024,
		HEIGHT		= 722,
		BPP			= 16
	};
}

LRESULT CALLBACK GLWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT	returnValue = 0;
	switch( uMsg )
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		break;

	case WM_KEYUP:
		break;

	case WM_CHAR:
		break;

	case WM_LBUTTONDOWN:
		{
			
		}
		break;

	case WM_LBUTTONUP:
		{
			
		}
		break;

	case WM_RBUTTONDOWN:
		{
			
		}
		break;

	case WM_RBUTTONUP:
		{
			
		}
		break;

	case WM_MOUSEMOVE:
		{
			
		}
		break;

	default:
		returnValue = DefWindowProc( hWnd, uMsg, wParam, lParam);
		break;
	}

	return returnValue;
}

// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;

	FileManager*	pFileManager = new FileManager("\\..\\..");

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, titleString, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MLGUI, classString, MAX_LOADSTRING);

	Window*			pMainWindow = new Window(hInstance, titleString, NULL);	
	pMainWindow->SetClassName(classString);
	pMainWindow->SetWindowProc(WndProc);
	pMainWindow->SetClassStyle(CS_HREDRAW | CS_VREDRAW);
	pMainWindow->SetMenuName(MAKEINTRESOURCE(IDC_MLGUI));
	pMainWindow->SetIcon(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MLGUI)));
	pMainWindow->SetSmallIcon(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL)));
	pMainWindow->Register();
	pMainWindow->Create();

	RECT	menuClientRect;
	BOOL	success = GetClientRect(pMainWindow->GetHandle(), &menuClientRect);
	Assert(success);
	
	int		menuClientWidth = menuClientRect.right - menuClientRect.left;
	int		menuClientHeight = menuClientRect.bottom - menuClientRect.top;

	//	init GL window
	GLWindow*		pGLWindow = new GLWindow(hInstance, L"OpenGL", pMainWindow->GetHandle());
	pGLWindow->SetWindowProc(GLWindowProc);
	pGLWindow->SetWindowStyle(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	pGLWindow->SetClassStyle(CS_OWNDC);
	//pGLWindow->SetWidth(WIDTH);
	//pGLWindow->SetHeight(HEIGHT);
	pGLWindow->SetWidth(menuClientWidth);
	pGLWindow->SetHeight(menuClientHeight);
	pGLWindow->SetBitsPerPixel(BPP);
	pGLWindow->Register();
	pGLWindow->Create();

	//	bone dict
	AnimGenBoneDictionary	boneDictionary;
	{
		bool	boneDictLoaded = boneDictionary.TryToLoadFromFile();
		Assert(boneDictLoaded);
	}

	//	main dialog
	int				currentX = 0;
	int				currentY = 0;
	//MainMenuDialog*	pMainMenuDialog = new MainMenuDialog(hInst, pMainWindow->GetHandle());
	MainMenuDialog*	pMainMenuDialog = new MainMenuDialog(hInst, pGLWindow->GetHandle());
	pMainMenuDialog->InitMenu(currentX, currentY);

	//
	Renderer::Init();
	LiveTextFont::InitLibrary();

	PreviewScene*	pPreviewScene = new PreviewScene();

	//
	HACCEL			hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MLGUI));
    MSG				msg;

	pGLWindow->Show();
	pMainWindow->Show();

	Clock			clock;
	bool			done = false;
	while( !done )
	{
		if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			if( msg.message == WM_QUIT )
			{
				done = TRUE;
			}
			else
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		else
		{
			f64	timeSlice = clock.GetTimeSlice();

			Renderer::PreRender();

			pMainMenuDialog->Update(timeSlice);
			pPreviewScene->Update(timeSlice);

			Renderer::PostRender();

			pGLWindow->SwapBuffers();
		}
	}

	delete pPreviewScene;

	LiveTextFont::ShutdownLibrary();
	Renderer::Shutdown();

	delete pMainMenuDialog;
	delete pGLWindow;
	delete pMainWindow;
	delete pFileManager;

    return (int)msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch(wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

			case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch(message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL )
        {
			EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
