#include "GameFlow.h"

#include "DebugMenu.h"
#include "TestRegressionGameState.h"
#include "TestFBXGameState.h"
#include "AnimationViewerState.h"

//
const char	*s_markerDescriptions[] =
{
	"Debug Menu",
	"Test Regression",
	"Test FBX",
	"Animation Viewer"
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

	case kTestRegression:
		returnValue = new TestRegressionGameState;
		break;

	case kTestFBX:
		returnValue = new TestFBXGameState;
		break;

	case kAnimViewer:
		returnValue = new AnimationViewerState;
		break;		

	default:
		_Panicf("Unhandled GameFlow marker %d\n", marker);
		break;
	};

	return returnValue;
}