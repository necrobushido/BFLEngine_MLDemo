#pragma once

#include "types.h"
#include "Vector2.h"

namespace Input
{
	struct MouseState
	{
		Vector2		m_pos;
		Vector2		m_prevPos;
		Vector2		m_posDelta;
		bool		m_leftPressed;
		bool		m_leftHeld;
		bool		m_rightPressed;
		bool		m_rightHeld;
	};

	bool KeyPressed(u8 key);
	bool KeyHeld(u8 key);
	bool KeyReleased(u8 key);

	const MouseState &GetMouseState();

	void ToggleKey(u8 key, bool down);
	void ToggleLeftMouse(bool down, int x, int y);
	void ToggleRightMouse(bool down, int x, int y);
	void SetMousePos(u32 x, u32 y);
	void AddText(u8 key);
	bool InputExists();
	const char *GetFrameInput();

	void Init();
	void Update();
}