#pragma once

#include "types.h"

//	a class that inherits from this should implement the "Run" method, and call "Start" to begin execution
//	it should then run until complete
class JobThread
{
protected:
	static unsigned __stdcall ThreadProc(void* param);

public:
	JobThread();
	virtual ~JobThread();

public:
	void Start();
	void Stop();

	void RunJob();
	bool IsJobRunning(){ return m_jobRunning; }

	HANDLE GetHandle(){ return m_handle; }

protected:
	bool IsRunning(){ return m_running; }	//	this doesn't correspond to the job, but rather the thread
	virtual void Run() = 0;
	unsigned int EndThread();
	void Cleanup();

protected:
	HANDLE	m_handle;
	u32		m_id;
	bool	m_running;
	bool	m_jobRunning;
	HANDLE	m_runJobAgainEvent;
};