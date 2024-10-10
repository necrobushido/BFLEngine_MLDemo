#include "GameFlow.h"

#include "DebugMenu.h"
#include "TestGameState.h"

//
const char	*s_markerDescriptions[] =
{
	"Debug Menu",
	"Test"
};

const char *GameFlow::GetDescription(eMarker marker)
{
	return s_markerDescriptions[marker];
}

GameState *GameFlow::GetGameState(s32 marker)
{
	GameState	*returnValue = NULL;
	switch(marker)
	{
	case kDebugMenu:
		returnValue = new DebugMenu;
		break;

	case kTestGameState:
		returnValue = new TestGameState;
		break;

	default:
		_Panicf("Unhandled GameFlow marker %d\n", marker);
		break;
	};

	return returnValue;
}