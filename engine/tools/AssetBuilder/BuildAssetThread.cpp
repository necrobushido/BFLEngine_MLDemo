#include "stdafx.h"

#include "BuildAssetThread.h"

namespace BuildAssetThread
{
	const int			kMaxThreads = 4;

	std::atomic<int>	s_currentThreads = 0;

	char				s_commandBuffers[kMaxThreads][PATH_BUFFER_SIZE];
	int					s_currentBufferIdx = 0;

	int ExecuteCommand(char* cmd) 
	{
		printf("%s\n", cmd);
			
		FILE*	pipe = _popen(cmd, "r");
    
		if( !pipe )
		{
			return 0;
		}

		//char	textOutput[PATH_BUFFER_SIZE * 10];
		//textOutput[0] = '\0';
		char	buffer[PATH_BUFFER_SIZE];
		while(!feof(pipe)) 
		{
    		if( fgets(buffer, PATH_BUFFER_SIZE, pipe) != NULL )
			{
				//strcat(textOutput, buffer);
				//fprintf(stdout, buffer);
			}
		}
		_pclose(pipe);

		//printf("%s", textOutput);
		return 1;
	}

	unsigned __stdcall ThreadProc(void* param)
	{
		char*	cmd = (char*)param;

		int	success = ExecuteCommand(cmd);
		assert(success);

		s_currentThreads--;

		return 0;
	}

	void ExecuteBuildCommand(const char* cmd)
	{
		while(s_currentThreads >= kMaxThreads)
		{
			//	spin lock
			const int	kSleepMilliseconds = 100;
			Sleep(kSleepMilliseconds);
		}

		int	ourDataIdx = s_currentBufferIdx;
		s_currentBufferIdx++;
		if( s_currentBufferIdx >= kMaxThreads )
		{
			s_currentBufferIdx = 0;
		}

		strcpy(s_commandBuffers[ourDataIdx], cmd);		

		HANDLE			handle;
		unsigned int	id;
		s_currentThreads++;
		handle = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, s_commandBuffers[ourDataIdx], 0, &id);
		assert(handle != NULL);
		CloseHandle(handle);	//	fire the thread off and forget about it
	}
};