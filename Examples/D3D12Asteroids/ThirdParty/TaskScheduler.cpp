////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////

/*!
    \file TaskScheduler.cpp

    TaskScheduler is a class that manages the distribution of task among the worker
    threads and the lifetime of the worker threads.

*/

#include <Windows.h>

#include "TaskMgr.h"
#include "TaskScheduler.h"

#include <new>

#pragma warning ( push )
#pragma warning ( disable : 4995 ) // skip deprecated warning on intrinsics.
#include <intrin.h>
#pragma warning ( pop )

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, char* threadName)
{
	// https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90)?redirectedfrom=MSDN
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}


  // helper function to get the number of processors on the system
DWORD get_proc_count()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

DWORD WINAPI TaskScheduler::ThreadMain(VOID* scheduler)
{
	TaskScheduler *pScheduler = reinterpret_cast<TaskScheduler*>(scheduler);
	pScheduler->ExecuteTasks();

	return 0;
}

// task_scheduler implementation

  // Initializes the scheduler and creates the worker threads
VOID TaskScheduler::Init(PROCESSOR_INFO& procInfo, CoreTypes coreType, int thread_count)
{   
	char buffer[255];

      // If the scheduler is still running, ignore this
    if(mbAlive == TRUE) return;

    muContextId = 0;
    mbAlive = TRUE;
    miWriter = 0;
    miTaskCount = 0;

      // Set the buffer of active tasks to empty by marking all of the slots as
      // TASKSETHANDLE_INVALID
    memset(mhActiveTaskSets,-1,sizeof(mhActiveTaskSets));

      // Get the number of worker threads that will be available
    if(thread_count == MAX_THREADS)
    {
          // Leave one core for the main thread.
        miThreadCount = procInfo.GetCoreTypeCount(coreType);
    }
    else
    {
        miThreadCount = thread_count;
    }

    if(miThreadCount == 0)
    {
        mpThreadData = 0;
        return;
    }

    mhTaskAvailable = CreateSemaphore(0,0,miThreadCount,0);

    // Create and initialize all of the threads
    mpThreadData = new HANDLE[miThreadCount];
    for(INT uThread = 0; uThread < miThreadCount; ++uThread)
    {
		switch (coreType)
		{
		case INTEL_ATOM:
			sprintf(buffer, "E-Core Thread %d", uThread);
			break;
		case INTEL_CORE:
			sprintf(buffer, "P-Core Thread %d", uThread);
			break;
		case ANY:
			sprintf(buffer, "Any Thread %d", uThread);
			break;
		default:
			sprintf(buffer, "Default Thread %d", uThread);
			break;
		}


		mpThreadData[uThread] = CreateThread(0, 0, TaskScheduler::ThreadMain, this, 0, 0);
		SetThreadName(GetThreadId(mpThreadData[uThread]), buffer);

#ifdef ENABLE_CPU_SETS
        RunOn(procInfo, mpThreadData[uThread], coreType, procInfo.cpuSets[CoreTypes::ANY]);
#else
        RunOn(procInfo, mpThreadData[uThread], coreType, procInfo.coreMasks[CoreTypes::ANY]);
#endif
    }
}
 
  // Clean up the worker threads and associated data
VOID TaskScheduler::Shutdown()
{
      // Tell of of the threads to break out of their loops
    mbAlive = FALSE;
      //Wake up a sleeping threads and wait for them to exit
    LONG sleep_count = 0;
    ReleaseSemaphore(mhTaskAvailable,1,&sleep_count);
    ReleaseSemaphore(mhTaskAvailable,miThreadCount - sleep_count,0);
    WaitForMultipleObjects(miThreadCount,mpThreadData,TRUE,INFINITE);

      // Clean up the handles
    for(INT uThread = 0; uThread < miThreadCount; ++uThread)
        CloseHandle(mpThreadData[uThread]);

    delete [] mpThreadData;
	mpThreadData = 0;
	miThreadCount = 0;
}

  // Main loop for the worker threads
VOID TaskScheduler::ExecuteTasks()
{
      // Get the ID for the thread
    const UINT iContextId = _InterlockedIncrement((LONG*)&muContextId);
      // Start reading from the beginning of the work queue
    INT  iReader = 0;

      // Thread keeps recieving and executing tasks until it is terminated
    while(mbAlive == TRUE)
    {
          // Get a Handle from the work queue
        TASKSETHANDLE handle = mhActiveTaskSets[iReader];

          // If there is a TaskSet in the slot execute a task
        if(handle != TASKSETHANDLE_INVALID)
        {
            TaskMgrSS::TaskSet *pSet = &gTaskMgrSS.mSets[handle];
            if(pSet->muCompletionCount > 0 && pSet->muTaskId >= 0)
            {
                pSet->Execute(iContextId);
            }
            else
            {
                _InterlockedCompareExchange((LONG*)&mhActiveTaskSets[iReader],TASKSETHANDLE_INVALID,handle);
                iReader = (iReader + 1) & (MAX_TASKSETS - 1);
            }
        }
          // Otherwise keep looking for work
        else if(miTaskCount > 0)
        {
            iReader = (iReader + 1) & (MAX_TASKSETS - 1);
        }
          // or sleep if all of the work has been completed

		// TODO: Steal any waiting work...
        else
        {
            if(miTaskCount <= 0)
            {
                WaitForSingleObject(mhTaskAvailable,INFINITE);
            }
        }
    }
}

  // Adds a task set to the work queue
VOID TaskScheduler::AddTaskSet( TASKSETHANDLE hSet, INT iTaskCount )
{
      // Increase the Task Count before adding the tasks to keep the
      // workers from going to sleep during this process
    _InterlockedExchangeAdd((LONG*)&miTaskCount,iTaskCount);

      // Looks for an open slot starting at the end of the queue
    INT iWriter = miWriter;
    do
    {
        while(mhActiveTaskSets[iWriter] != TASKSETHANDLE_INVALID)
            iWriter = (iWriter + 1) & (MAX_TASKSETS - 1);

        // verify that another thread hasn't already written to this slot
    } while(_InterlockedCompareExchange((LONG*)&mhActiveTaskSets[iWriter],hSet,TASKSETHANDLE_INVALID) != TASKSETHANDLE_INVALID);

      // Wake up all suspended threads
    LONG sleep_count = 0;
    ReleaseSemaphore(mhTaskAvailable,1,&sleep_count);
    INT iCountToWake = iTaskCount < (miThreadCount - sleep_count - 1) ? iTaskCount : miThreadCount - sleep_count - 1;
    ReleaseSemaphore(mhTaskAvailable,iCountToWake,&sleep_count);

      // reset the end of the queue
    miWriter = iWriter;
}

  // Yields the main thread to the scheduler when it needs to wait for a Task Set to be completed
VOID TaskScheduler::WaitForFlag( volatile BOOL *pFlag )
{
      // Start at the the end of the work queue
    int iReader = miWriter;

      // The condition for exiting this loop is changed externally to the function,
      // possibly in another thread.  The loop will break with no more than one task
      // being executed, returning the main thread as soon as possible.
    while(*pFlag == FALSE)
    {
		TASKSETHANDLE handle = mhActiveTaskSets[iReader];

        if(handle != TASKSETHANDLE_INVALID)
        {
            TaskMgrSS::TaskSet *pSet = &gTaskMgrSS.mSets[handle];
            if(pSet->muCompletionCount > 0 && pSet->muTaskId >= 0)
            {
                  // The context ID for the main thread is 0.
                pSet->Execute(0);
            }
            else
            {
                _InterlockedCompareExchange((LONG*)&mhActiveTaskSets[iReader],TASKSETHANDLE_INVALID,handle);
                iReader = (iReader + 1) & (MAX_TASKSETS - 1);
            }
        }
        else if(miTaskCount > 0)
        {
            iReader = (iReader + 1) & (MAX_TASKSETS - 1);
        }
        else
        {
              // Worker threads get suspended, but the main thread needs to stay alert,
              // so it spins until the condition is met or more work it added.
            while(miTaskCount == 0 && *pFlag == FALSE);
        }
    }
}
