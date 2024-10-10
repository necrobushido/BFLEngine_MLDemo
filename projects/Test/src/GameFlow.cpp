#include "GameFlow.h"

#include "GameState.h"
#include "Input.h"

GameFlow	*g_gameFlow = NULL;

GameFlow::GameFlow():
	m_pCurrentState(NULL),
	m_currentMarker(-1),
	m_desiredMarker(kDebugMenu)
{
	Assert(g_gameFlow == NULL);
	g_gameFlow = this;
}

GameFlow::~GameFlow()
{
	delete m_pCurrentState;
	g_gameFlow = NULL;
}

void GameFlow::Update(f64 deltaTime)
{
	if( Input::KeyPressed('H') )
	{
		Transition(kDebugMenu);
	}

	if( m_currentMarker != m_desiredMarker )
	{
		delete m_pCurrentState;
		m_pCurrentState = GetGameState(m_desiredMarker);
		m_currentMarker = m_desiredMarker;
	}

	if( m_pCurrentState )
	{
		m_pCurrentState->Update(deltaTime);
	}

	//	debug key for printing out a memory dump
	if( Input::KeyPressed('M') )
	{
		//Memory::DumpHeap();
	}
}

void GameFlow::ResizeCallback(s32 width, s32 height)
{
	if( m_pCurrentState )
	{
		m_pCurrentState->ResizeCallback(width, height);
	}
}

void GameFlow::Transition(eMarker marker)
{
	m_desiredMarker = marker;
}

bool GameFlow::ShouldExitGame()
{
	return Input::KeyPressed(VK_ESCAPE);
	//return false;
}