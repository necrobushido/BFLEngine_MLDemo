#pragma once

#include "types.h"

class GameState;

class GameFlow
{
public:
	enum eMarker
	{
		kDebugMenu,
		kTestGameState,

		kNumMarkers
	};

public:
	static const char* GetDescription(eMarker marker);

public:
	GameFlow();
	~GameFlow();

public:
	void Update(f64 deltaTime);
	void ResizeCallback(s32 width, s32 height);
	void Transition(eMarker marker);
	bool ShouldExitGame();

private:
	GameState* GetGameState(s32 marker);

private:
	GameState*	m_pCurrentState;
	s32			m_currentMarker;
	s32			m_desiredMarker;	
};

extern GameFlow*	g_gameFlow;