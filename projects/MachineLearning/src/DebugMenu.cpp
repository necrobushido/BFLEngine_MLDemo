#include "DebugMenu.h"

#include "GameFlow.h"
#include "Input.h"
#include "Renderer.h"
#include "DefaultShaderProgram.h"

namespace
{
	const float kZFar = 5000.0f;
	const float kZNear = 0.1f;

	const char*	s_font = "arial.ttf";
}

DebugMenu::DebugMenu():
	m_font(s_font, 1 << 10),
	m_currentlySelected(1)
{
	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

DebugMenu::~DebugMenu()
{
}

void DebugMenu::Update(f64 deltaTime)
{
	//	check for transition
	if( Input::KeyPressed(VK_RETURN) )
	{
		g_gameFlow->Transition((GameFlow::eMarker)m_currentlySelected);
	}

	//	selection
	if( Input::KeyPressed('W') )
	{
		m_currentlySelected--;
	}
	if( Input::KeyPressed('S') )
	{
		m_currentlySelected++;
	}
	if( m_currentlySelected >= GameFlow::kNumMarkers )
	{
		m_currentlySelected -= GameFlow::kNumMarkers - 1;
	}
	if( m_currentlySelected < 1 )
	{
		m_currentlySelected += GameFlow::kNumMarkers - 1;
	}

	//	draw the menu
	m_orthoCamera.Apply();
	
	Vector3		numberPos(0.0f, (f32)m_screenHeight - m_font.GetTextHeight(), -10.0f);
	Vector3		currentEntryPos = numberPos;
	currentEntryPos.x += 32.0f;

	const Color4	white(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	red(1.0f, 0.0f, 0.0f, 1.0f);

	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	//	start at 1 because 0 is the Debug Menu itself
	for(int i = 1; i < GameFlow::kNumMarkers; i++)
	{
		const Color4	*currentColor = &white;
		if( i == m_currentlySelected )
		{
			currentColor = &red;
		}

		char	numberBuffer[32];
		sprintf(numberBuffer, "%d", i);
		m_font.Draw(numberBuffer, numberPos, *currentColor, OrthoFont::kJustify_Left);
		m_font.Draw(GameFlow::GetDescription((GameFlow::eMarker)i), currentEntryPos, *currentColor, OrthoFont::kJustify_Left);

		numberPos.y -= m_font.GetTextHeight();
		currentEntryPos.y -= m_font.GetTextHeight();
	}
}

void DebugMenu::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}