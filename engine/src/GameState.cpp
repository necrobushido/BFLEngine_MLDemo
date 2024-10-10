#include "GameState.h"

GameState::GameState()
{
	GLint	viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	m_screenWidth = (f32)viewport[2];
	m_screenHeight = (f32)viewport[3];
}

GameState::~GameState()
{
}