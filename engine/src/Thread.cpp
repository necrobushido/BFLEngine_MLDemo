#include "Thread.h"

unsigned __stdcall Thread::ThreadProc(void* param)
{
	Thread*	pCallingThread = (Thread*)param;

	while(pCallingThread->IsRunning())
	{
		pCallingThread->Run();
	}

	return pCallingThread->EndThread();
}

//DWORD WINAPI Thread::ThreadProc(void* param)
//{
//	Thread*	pCallingThread = (Thread*)param;
//
//	while(pCallingThread->IsRunning())
//	{
//		pCallingThread->Run();
//	}
//
//	return pCallingThread->EndThread();
//}

Thread::Thread():
	m_handle(NULL),
	m_running(false),
	m_suspended(false)
{
}

Thread::~Thread()
{
	WaitForSingleObject(m_handle, INFINITE);
	CloseHandle(m_handle);
}

void Thread::Start()
{
	Assert(!m_running);
	m_running = true;

	//LPDWORD	threadID;
	//m_handle = CreateThread(NULL, 0, ThreadProc, this, 0, threadID);		//	C run-time isn't thread safe with this?
	//m_id = *threadID;
	m_handle = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, &m_id);
	Assert(m_handle != NULL);
}

void Thread::Stop()
{
	//	does this need a mutex?
	//		the current assumption is that after the thread starts m_running should only ever be changed to false, so race conditions shouldn't apply
	m_running = false;
}

void Thread::Suspend()
{
	DWORD	previousSuspendCount = SuspendThread(m_handle);
	Assert(previousSuspendCount != (DWORD)-1);

	m_suspended = true;
}

void Thread::Resume()
{
	DWORD	previousSuspendCount = ResumeThread(m_handle);
	Assert(previousSuspendCount != (DWORD)-1);

	if( previousSuspendCount == 1 )
	{
		m_suspended = false;
	}
}

void Thread::Wait(u32 milliseconds)
{
	Sleep(milliseconds);
}

unsigned int Thread::EndThread()
{
	const unsigned int	kReturnCode = 0;

	_endthreadex(kReturnCode);
	return kReturnCode;
}