#pragma once

#include "types.h"

class Mutex
{
public:
	Mutex();
	~Mutex();

public:
	void Enter();
	void Exit();

protected:
	CRITICAL_SECTION	m_criticalSection;
};

//	scoped mutex lock
class ScopedLock
{
public:
	ScopedLock(Mutex& pTargetMutex):
		m_pTargetMutex(&pTargetMutex)
	{
		m_pTargetMutex->Enter();
	}

	~ScopedLock()
	{
		m_pTargetMutex->Exit();
	}

	Mutex*	m_pTargetMutex;
};