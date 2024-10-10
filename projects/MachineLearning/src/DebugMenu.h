#pragma once

#include "types.h"
#include "GameState.h"
#include "CameraBase.h"
#include "OrthoFont.h"

class DebugMenu : public GameState
{
public:
	DebugMenu();
	virtual ~DebugMenu();

public:
	virtual void Update(f64 deltaTime);
	virtual void ResizeCallback(s32 width, s32 height);

protected:
	CameraBase	m_orthoCamera;
	OrthoFont	m_font;
	s32			m_currentlySelected;
};