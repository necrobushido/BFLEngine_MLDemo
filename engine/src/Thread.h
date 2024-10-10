#pragma once

#include "types.h"

//	a class that inherits from this should implement the "Run" method, and call "Start" to begin execution
//	to terminate the thread, usually you should have it's "Run" method call "Stop"
class Thread
{
protected:
	static unsigned __stdcall ThreadProc(void* param);

public:
	Thread();
	virtual ~Thread();

public:
	void Start();
	void Stop();

	void Suspend();
	void Resume();

	void Wait(u32 milliseconds);

	HANDLE GetHandle(){ return m_handle; }
	bool IsRunning(){ return m_running; }
	bool IsSuspended(){ return m_suspended; }

protected:
	virtual void Run() = 0;
	unsigned int EndThread();

protected:
	HANDLE	m_handle;
	u32		m_id;
	bool	m_running;
	bool	m_suspended;
};