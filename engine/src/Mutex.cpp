#include "Mutex.h"

Mutex::Mutex()
{
	InitializeCriticalSection(&m_criticalSection);	
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&m_criticalSection);
}

void Mutex::Enter()
{
	EnterCriticalSection(&m_criticalSection);
}

void Mutex::Exit()
{
	LeaveCriticalSection(&m_criticalSection);
}