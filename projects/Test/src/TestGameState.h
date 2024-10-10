#pragma once

#include "types.h"
#include "GameState.h"
#include "CameraBase.h"
#include "OrthoFont.h"

class TestGameState : public GameState
{
public:
	TestGameState();
	virtual ~TestGameState();

public:
	virtual void Update(f64 deltaTime) override;
	virtual void ResizeCallback(s32 width, s32 height);

protected:
	CameraBase	m_orthoCamera;
	OrthoFont	m_font;
};