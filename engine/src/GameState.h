#pragma once

#include "types.h"

class GameState
{
public:
	GameState();
	virtual ~GameState();

public:
	virtual void Update(f64 deltaTime) = 0;	//	deltaTime is in seconds
	virtual void ResizeCallback(s32 width, s32 height)
	{
		m_screenWidth = (f32)width;
		m_screenHeight = (f32)height;
	}

protected:
	f32	m_screenWidth;
	f32	m_screenHeight;
};