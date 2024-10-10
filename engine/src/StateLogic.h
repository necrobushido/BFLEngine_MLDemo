#pragma once

#include "types.h"

class StateLogic
{
public:
	StateLogic():
		m_currentState(-1),
		m_oldState(-1),
		m_stateTime(0.0f),
		m_prevStateTime(0.0f)
	{
	}

public:
	void Update(f64 deltaTime)
	{
		m_prevStateTime = m_stateTime;
		m_stateTime += deltaTime;
		m_oldState = m_currentState;
	}

	u32 GetState()
	{ 
		return m_currentState; 
	}

	void SetState(u32 state, bool ignoreCurrentState = false)
	{
		if( m_currentState != state || ignoreCurrentState )
		{
			m_currentState = state;
			m_stateTime = 0.0f;
		}
	}

	void FlagNewState()
	{
		m_oldState = -1;
		m_stateTime = 0.0f;
	}

	f64 GetPrevStateTime()
	{
		return m_prevStateTime;
	}

	f64 GetStateTime()
	{
		return m_stateTime;
	}

	void SetStateTime(f64 time)
	{
		m_stateTime = time;
	}

	bool IsNewState()
	{
		return m_currentState != m_oldState;
	}

	u32 GetPrevState()
	{ 
		return m_oldState; 
	}

	bool CrossedTime(f64 time)
	{
		return m_stateTime >= time && m_prevStateTime < time;
	}

protected:
	u32	m_currentState;
	u32	m_oldState;
	f64	m_stateTime;
	f64	m_prevStateTime;
};