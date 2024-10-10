#include "MainMenuDialog.h"

#include "Resource.h"

#include <Commdlg.h>
#include <Commctrl.h>

#include "AnimGenProcThread.h"
#include "PreviewScene.h"
#include "NNDDPMSampler.h"

MainMenuDialog*	g_mainMenuDialog = NULL;

MainMenuDialog::MainMenuDialog(HINSTANCE hInst, HWND hParent):
	m_dialog(hInst, IDD_DIALOGMAIN, hParent),
	m_pAnimGenProcThread(nullptr),
	m_prevThreadState(-1)
{
	Assert(g_mainMenuDialog == NULL);
	g_mainMenuDialog = this;

	m_dialog.SetWindowProc(DialogProcedure);

	m_pAnimGenProcThread = new AnimGenProcThread();
	m_pAnimGenProcThread->SetLogFunc(DebugPrint);
	m_pAnimGenProcThread->Start();
}

MainMenuDialog::~MainMenuDialog()
{
	m_pAnimGenProcThread->Stop();
	delete m_pAnimGenProcThread;
	m_pAnimGenProcThread = nullptr;

	g_mainMenuDialog = NULL;
}

void MainMenuDialog::InitMenu(int x, int y)
{
	m_dialog.Create();
	m_dialog.SetPos(x, y);
	m_dialog.Show();

	//	CFG
	{
		HWND		hCFGEditBox = GetDlgItem(m_dialog.GetHandle(), IDC_CFGEDIT);
		wchar_t		wCFGText[256];
		swprintf(wCFGText, ARRAY_SIZE(wCFGText), L"%.9g", 7.5f);
		SendMessage(hCFGEditBox, WM_SETTEXT, 0, (LPARAM)wCFGText);
	}

	//	inference steps
	{
		HWND		hInfStepsEditBox = GetDlgItem(m_dialog.GetHandle(), IDC_INFSTEPSEDIT);
		wchar_t		wInfStepsText[256];
		swprintf(wInfStepsText, ARRAY_SIZE(wInfStepsText), L"%d", g_NNDDPMSampler->GetTrainingStepCount());
		SendMessage(hInfStepsEditBox, WM_SETTEXT, 0, (LPARAM)wInfStepsText);
	}
}

int MainMenuDialog::GetHeight()
{
	return m_dialog.GetHeight();
}

void MainMenuDialog::Update(f64 deltaTime)
{
	int	currentThreadState = m_pAnimGenProcThread->GetState();

	if( currentThreadState != m_prevThreadState )
	{
		if( m_prevThreadState == NNProcessingThreadBase::kProcessState_Generating &&
			currentThreadState == NNProcessingThreadBase::kProcessState_Idle )
		{
			//	an animation was just generated and the thread went to idle, so we should be good to copy the animation over here and use it

			AnimationData*	pTargetAnimData = g_PreviewScene->GetAnimDataPtr();
			
			int	currentTestAnimsActive = m_pAnimGenProcThread->ExportGeneratedAnims(pTargetAnimData, PreviewScene::kMaxAnimationsToSample);
			g_PreviewScene->SetActiveAnimCount(currentTestAnimsActive);

			g_PreviewScene->SetupGeneratedAnims();
		}

		if( currentThreadState != NNProcessingThreadBase::kProcessState_Idle )
		{
			HWND	hSubControl;
			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_GENERATEBUTTON);
			EnableWindow(hSubControl, FALSE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_PROMPTEDIT);
			EnableWindow(hSubControl, FALSE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_CFGEDIT);
			EnableWindow(hSubControl, FALSE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_INFSTEPSEDIT);
			EnableWindow(hSubControl, FALSE);
		}
		else
		{
			HWND	hSubControl;
			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_GENERATEBUTTON);
			EnableWindow(hSubControl, TRUE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_PROMPTEDIT);
			EnableWindow(hSubControl, TRUE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_CFGEDIT);
			EnableWindow(hSubControl, TRUE);

			hSubControl = GetDlgItem(m_dialog.GetHandle(), IDC_INFSTEPSEDIT);
			EnableWindow(hSubControl, TRUE);
		}

		m_prevThreadState = currentThreadState;
	}
}

void MainMenuDialog::GenerateButtonPressed()
{
	//	set the prompt
	{
		HWND		hPromptEditBox = GetDlgItem(m_dialog.GetHandle(), IDC_PROMPTEDIT);
		int			promptTextLength = GetWindowTextLength(hPromptEditBox) + 1;
		wchar_t*	wPromptText = new wchar_t[promptTextLength];
		SendMessage(hPromptEditBox, WM_GETTEXT, promptTextLength, (LPARAM)wPromptText);

		char*		promptText = new char[promptTextLength];
		wcstombs(promptText, wPromptText, promptTextLength);

		m_pAnimGenProcThread->SetPrompt(promptText);

		delete [] promptText;
		delete [] wPromptText;
	}

	//	set CFG
	//		we need the input to this box to be validated to ensure only float values, but I'm lazy atm
	{
		HWND		hCFGEditBox = GetDlgItem(m_dialog.GetHandle(), IDC_CFGEDIT);
		int			cfgTextLength = GetWindowTextLength(hCFGEditBox) + 1;
		wchar_t*	wCFGText = new wchar_t[cfgTextLength];
		SendMessage(hCFGEditBox, WM_GETTEXT, cfgTextLength, (LPARAM)wCFGText);

		char*		cfgText = new char[cfgTextLength];
		wcstombs(cfgText, wCFGText, cfgTextLength);

		double		cfgValue = atof(cfgText);

		delete [] cfgText;
		delete [] wCFGText;

		m_pAnimGenProcThread->SetCFG(cfgValue);
	}

	//	set inference timesteps
	{
		HWND		hInfStepsEditBox = GetDlgItem(m_dialog.GetHandle(), IDC_INFSTEPSEDIT);
		int			infStepsTextLength = GetWindowTextLength(hInfStepsEditBox) + 1;
		wchar_t*	wInfStepsText = new wchar_t[infStepsTextLength];
		SendMessage(hInfStepsEditBox, WM_GETTEXT, infStepsTextLength, (LPARAM)wInfStepsText);

		char*		infStepsText = new char[infStepsTextLength];
		wcstombs(infStepsText, wInfStepsText, infStepsTextLength);

		int			infStepsValue = atoi(infStepsText);

		delete [] infStepsText;
		delete [] wInfStepsText;

		if( infStepsValue < 1 )
		{
			infStepsValue = 1;
		}
		if( infStepsValue > g_NNDDPMSampler->GetTrainingStepCount() )
		{
			//	going larger than this causes multiplication by zero
			infStepsValue = g_NNDDPMSampler->GetTrainingStepCount();
		}
		g_NNDDPMSampler->SetInferenceTimesteps(infStepsValue);
	}

	//	reset progress bar
	{
		HWND		hProgressBar = GetDlgItem(m_dialog.GetHandle(), IDC_GENPROGRESS);

		int			DDPMGenerationSteps = g_NNDDPMSampler->GetInferenceTimestepCount();
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, DDPMGenerationSteps));    
		SendMessage(hProgressBar, PBM_SETSTEP, (WPARAM)1, 0); 		
		SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
	}
	
	//
	m_pAnimGenProcThread->SetGenProgressFunc(UpdateProgressBar);
	m_pAnimGenProcThread->UpdateTargetModel("TrainingData\\DefaultBot\\DefaultBot.mdl");
	m_pAnimGenProcThread->SetSampleCount(PreviewScene::kMaxAnimationsToSample);
	m_pAnimGenProcThread->ChangeState(NNProcessingThreadBase::kProcessState_Generating);
}

//	static
INT_PTR CALLBACK MainMenuDialog::DialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR	returnValue = FALSE;
	switch(msg)
	{
	case WM_INITDIALOG:
		returnValue = TRUE;
		break;

	case WM_COMMAND:
		{
			int	wmId    = LOWORD(wParam);
			int	wmEvent = HIWORD(wParam);
			switch(wmId)
			{
			case IDC_DATASETEDIT:
				switch(wmEvent)
				{
				case EN_CHANGE:
					{
					}
					break;

				default:
					returnValue = DefWindowProc(hwnd, msg, wParam, lParam);
					break;
				}
				break;

			case IDC_GENERATEBUTTON:
				{
					g_mainMenuDialog->GenerateButtonPressed();
				}
				break;

			default:
				returnValue = DefWindowProc(hwnd, msg, wParam, lParam);
				break;
			}
		}
		break;

	default:
		returnValue = DefWindowProc(hwnd, msg, wParam, lParam);
		break;
	}

	return returnValue;
}

void MainMenuDialog::UpdateProgressBar()
{
	Assert(g_mainMenuDialog != nullptr);

	HWND		hProgressBar = GetDlgItem(g_mainMenuDialog->m_dialog.GetHandle(), IDC_GENPROGRESS);
	SendMessage(hProgressBar, PBM_STEPIT, 0, 0); 
}