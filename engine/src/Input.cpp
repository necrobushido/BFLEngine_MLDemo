#include "Input.h"

namespace Input
{
	enum
	{
		kNumKeys = 256
	};

	bool		m_keysPressed[kNumKeys];
	bool		m_keysHeld[kNumKeys];
	bool		m_keysReleased[kNumKeys];
	MouseState	m_mouseState;
	char		m_frameInput[kNumKeys];		//	string of keys pressed during the frame

	bool KeyPressed(u8 key)
	{
		return m_keysPressed[key];
	}

	bool KeyHeld(u8 key)
	{
		return m_keysHeld[key];
	}

	bool KeyReleased(u8 key)
	{
		return m_keysReleased[key];
	}

	const MouseState &GetMouseState()
	{
		return m_mouseState;
	}

	void ToggleKey(u8 key, bool down)
	{
		if( down )
		{
			m_keysPressed[key] = true;
			m_keysHeld[key] = true;
		}
		else
		{
			m_keysHeld[key] = false;
			m_keysReleased[key] = true;
		}
	}

	void ToggleLeftMouse(bool down, int x, int y)
	{
		m_mouseState.m_leftPressed = down;
		m_mouseState.m_leftHeld = down;
		SetMousePos(x, y);
		m_mouseState.m_prevPos = m_mouseState.m_pos;
	}

	void ToggleRightMouse(bool down, int x, int y)
	{
		m_mouseState.m_rightPressed = down;
		m_mouseState.m_rightHeld = down;
		SetMousePos(x, y);
		m_mouseState.m_prevPos = m_mouseState.m_pos;
	}

	void SetMousePos(u32 x, u32 y)
	{
		m_mouseState.m_pos.x = (coord_type)x;
		m_mouseState.m_pos.y = (coord_type)y;

		m_mouseState.m_posDelta = m_mouseState.m_pos - m_mouseState.m_prevPos;
	}

	void AddText(u8 key)
	{
		char	charString[2];
		charString[0] = (char)key;
		charString[1] = '\0';
		strcat(m_frameInput, charString);
	}

	bool InputExists()
	{
		return m_frameInput[0] != '\0';
	}

	const char *GetFrameInput()
	{
		return m_frameInput;
	}

	void Init()
	{
		memset( m_keysPressed, 0, sizeof(bool) * kNumKeys );
		memset( m_keysHeld, 0, sizeof(bool) * kNumKeys );
		memset( m_keysReleased, 0, sizeof(bool) * kNumKeys );
		m_frameInput[0] = '\0';
	}

	void Update()
	{
		for(u16 i = 0; i < kNumKeys; i++)
		{
			m_keysPressed[i] = false;
			m_keysReleased[i] = false;
		}

		m_mouseState.m_leftPressed = false;
		m_mouseState.m_rightPressed = false;
		m_mouseState.m_prevPos = m_mouseState.m_pos;
		m_mouseState.m_posDelta = Vector2::ZERO;
		m_frameInput[0] = '\0';
	}
}