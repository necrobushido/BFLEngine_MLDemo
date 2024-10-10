#pragma once

#include "WindowsDialog.h"

class AnimGenProcThread;

class MainMenuDialog
{
public:
	MainMenuDialog(HINSTANCE hInst, HWND hParent);
	~MainMenuDialog();

public:
	void InitMenu(int x, int y);
	int GetHeight();

	void Update(f64 deltaTime);

protected:
	void GenerateButtonPressed();

private:
	WindowsDialog					m_dialog;

	AnimGenProcThread*				m_pAnimGenProcThread;
	int								m_prevThreadState;

public:
	static INT_PTR CALLBACK DialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static void UpdateProgressBar();
};

extern MainMenuDialog*	g_mainMenuDialog;