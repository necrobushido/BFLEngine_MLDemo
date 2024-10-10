#include "framework.h"
#include "Main.h"

#include "Input.h"
#include "GameWindow.h"
#include "Clock.h"
#include "FileManager.h"
#include "Renderer.h"
#include "LiveTextFont.h"

#include "GameFlow.h"

namespace
{
	enum
	{
		SIZE_DENOM		= 1,
		WIDTH			= 1900 / SIZE_DENOM,
		HEIGHT			= 1280 / SIZE_DENOM,
		BPP				= 32,
		HEAP_SIZE		= 500 * (1 << 20),
		SOUND_HEAP_SIZE	= 2 * (1 << 20)			//	2 megs
	};

	const wchar_t*	gameTitle = L"Test";
};

void BaseResizeCallback(GLsizei width, GLsizei height)
{
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	//void*			pHeap = malloc(HEAP_SIZE);
	//Memory::Init(pHeap, HEAP_SIZE);

	FileManager*	pFileManager = new FileManager("\\..\\..");	//	create singleton FileManager

	Input::Init();

	GLWindow::SetResizeCallback(BaseResizeCallback);
	GameWindow*		pGameWindow = new GameWindow(hInstance, gameTitle, NULL);
	pGameWindow->SetWidth(WIDTH);
	pGameWindow->SetHeight(HEIGHT);
	pGameWindow->SetBitsPerPixel(BPP);
	pGameWindow->Register();
	pGameWindow->Create();
	pGameWindow->Show();

	Renderer::Init();
	LiveTextFont::InitLibrary();

	GameFlow*		pGameFlow = new GameFlow();

	MSG				msg;
	bool			done = false;
    Clock			clock;
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
			
			if( pGameWindow->GetActive() )
			{
				Renderer::PreRender();

				pGameFlow->Update(timeSlice);			
				
				Renderer::PostRender();
				
				pGameWindow->SwapBuffers();

				done = pGameFlow->ShouldExitGame();

				Input::Update();
			}
		}
	}

	delete pGameFlow;
	LiveTextFont::ShutdownLibrary();
	Renderer::Shutdown();
	delete pGameWindow;
	delete pFileManager;

	//	make sure to delete everything before Memory shuts down
	//Memory::Shutdown();
	//free(pHeap);

    return (int) msg.wParam;
}