#include "TestGameState.h"

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

TestGameState::TestGameState():
	m_font(s_font, 1 << 10)
{
	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}

TestGameState::~TestGameState()
{
}

void TestGameState::Update(f64 deltaTime)
{
	//	draw the menu
	m_orthoCamera.Apply();
	
	Vector3		numberPos(0.0f, (f32)m_screenHeight - m_font.GetTextHeight(), -10.0f);
	Vector3		currentEntryPos = numberPos;
	currentEntryPos.x += 32.0f;

	const Color4	white(1.0f, 1.0f, 1.0f, 1.0f);
	const Color4	red(1.0f, 0.0f, 0.0f, 1.0f);

	DefaultShaderProgram*	pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	const Color4*	currentColor = &white;
	m_font.Draw("omg!", numberPos, *currentColor, OrthoFont::kJustify_Left);
}

void TestGameState::ResizeCallback(s32 width, s32 height)
{
	GameState::ResizeCallback(width, height);

	m_orthoCamera.SetOrtho(0, m_screenWidth, m_screenHeight, 0, kZNear, kZFar);
}