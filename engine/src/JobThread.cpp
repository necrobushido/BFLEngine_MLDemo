#include "JobThread.h"

unsigned __stdcall JobThread::ThreadProc(void* param)
{
	JobThread*	pCallingThread = (JobThread*)param;

	while(pCallingThread->IsRunning())
	{
		//	wait for a call to RunJob before doing anything, even the first time
		//WaitForSingleObject(pCallingThread->m_runJobAgainEvent, INFINITE);

		pCallingThread->m_jobRunning = true;
		pCallingThread->Run();
		pCallingThread->m_jobRunning = false;

		WaitForSingleObject(pCallingThread->m_runJobAgainEvent, INFINITE);
	}

	return pCallingThread->EndThread();
}

JobThread::JobThread():
	m_handle(NULL),
	m_id(0),
	m_running(false),
	m_jobRunning(false)
{
	m_runJobAgainEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

JobThread::~JobThread()
{
	Stop();
	Cleanup();
}

void JobThread::Start()
{
	Assert(!m_running);
	m_running = true;

	Cleanup();

	m_handle = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, &m_id);
	Assert(m_handle != NULL);
}

void JobThread::Stop()
{
	m_running = false;
	SetEvent(m_runJobAgainEvent);	//	sends the signal to WaitForSingleObject in ThreadProc
}

void JobThread::RunJob()
{
	SetEvent(m_runJobAgainEvent);
}

unsigned int JobThread::EndThread()
{
	const unsigned int	kReturnCode = 0;

	_endthreadex(kReturnCode);
	return kReturnCode;
}

void JobThread::Cleanup()
{
	WaitForSingleObject(m_handle, INFINITE);
	CloseHandle(m_handle);
	m_handle = NULL;
}