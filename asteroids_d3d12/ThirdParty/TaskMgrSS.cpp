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
    \file TaskMgrSS.h

    TaksMgrSS is a class that uses a custom Windows threads schedulers with a
    C-style handle and callback mechanism for scheduling tasks across any 
    number of CPU cores.

*/
#include "SampleComponents.h"
#include "TaskMgrSS.h"

#include "TaskScheduler.h"
#include "spin_mutex.h"

#include <strsafe.h>

#pragma warning ( push )
#pragma warning ( disable : 4995 ) // skip deprecated warning on intrinsics.
#include <intrin.h>
#pragma warning ( pop )


//
//  Global Simple Scheduler task mananger instance
//
TaskMgrSS                      gTaskMgrSS;


TaskMgrSS::TaskSet::TaskSet() 
: mpFunc( NULL )
, mpvArg( 0 )
, muSize( 0 )
, mhTaskset( TASKSETHANDLE_INVALID )
, mbCompleted( TRUE )
{
    mszSetName[ 0 ] = 0;
    memset( Successors, 0, sizeof( Successors ) ) ;
};

void TaskMgrSS::TaskSet::Execute(INT iContextId)
{
    int uIdx = _InterlockedDecrement(&muTaskId);
    if(uIdx >= 0)
    {
        //gTaskMgrSS.mTaskScheduler.DecrementTaskCount();

        //ProfileBeginTask( mszSetName );

        mpFunc( mpvArg, iContextId, uIdx, muSize );

       // ProfileEndTask();

        //gTaskMgr.CompleteTaskSet( mhTaskset );
        UINT uCount = _InterlockedDecrement( (LONG*)&muCompletionCount );

        if( 0 == uCount )
        {
            mbCompleted = TRUE;
            mpFunc = 0;
            CompleteTaskSet();
        }
    }
}

void TaskMgrSS::TaskSet::CompleteTaskSet()
{
    //
    //  The task set has completed.  We need to look at the successors
    //  and signal them that this dependency of theirs has completed.
    //

    mSuccessorsLock.aquire();

    for( UINT uSuccessor = 0; uSuccessor < MAX_SUCCESSORS; ++uSuccessor )
    {
        TaskSet* pSuccessor = Successors[ uSuccessor ];

        //
        //  A signaled successor must be removed from the Successors list 
        //  before the mSuccessorsLock can be released.
        //
        Successors[ uSuccessor ] = NULL;
            
        if( NULL != pSuccessor ) 
        {
            UINT uStart;

            uStart = _InterlockedDecrement( (LONG*)&pSuccessor->muStartCount );

            //
            //  If the start count is 0 the successor has had all its 
            //  dependencies satisfied and can be scheduled.
            //
            if (0 == uStart)
            {
                if (gTaskMgrSS.mProcInfo.hybrid)
                {
#if CORE_ONLY
                    gTaskMgrSS.mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#else
                    if (pSuccessor->mCoreType == CoreTypes::INTEL_ATOM)
                    {
                        gTaskMgrSS.mAtomTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                    }
                    else if (pSuccessor->mCoreType == CoreTypes::INTEL_CORE)
                    {
                        gTaskMgrSS.mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                    }
                    else if (pSuccessor->mCoreType == CoreTypes::ANY)
                    {
#if RESERVE_ANY
                        gTaskMgrSS.mAnyTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#else
                        gTaskMgrSS.mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#endif
                    }
                    else
                    {
                        gTaskMgrSS.mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                    }
#endif
                }
                else
                {
                    gTaskMgrSS.mTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                }
            }
        }
    }

    mSuccessorsLock.release();

    gTaskMgrSS.ReleaseHandle( mhTaskset );
}

///////////////////////////////////////////////////////////////////////////////
//
//  Implementation of TaskMgrSS
//
///////////////////////////////////////////////////////////////////////////////

TaskMgrSS::TaskMgrSS() : miDemoModeThreadCountOverride(-1)
{
    memset(
        mSets,
        0x0,
        sizeof( mSets ) );
}

TaskMgrSS::~TaskMgrSS()
{
}

BOOL TaskMgrSS::Init(PROCESSOR_INFO& procInfo)
{
    mProcInfo = procInfo;

    if (mProcInfo.hybrid)
	{
        printf("%s\n\r", procInfo.brandString);
        printf("Logical Cores %d(%d Core(s)/%d Atom(s))\n\r", procInfo.numLogicalCores, (int)procInfo.cpuSets[CoreTypes::INTEL_CORE].size(), (int)procInfo.cpuSets[CoreTypes::INTEL_ATOM].size());
#if CORE_ONLY
        // Reserve 1 thread for main thread
        mCoreTaskScheduler.Init(procInfo, CoreTypes::INTEL_CORE, (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 1);
        printf("Initialized Core-Only Heterogeneous Threadpool (%d Threads)\n\r", (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 1);
#else
#if RESERVE_ANY
        // Allocate 2 threads for 'any' threadpool
        mAnyTaskScheduler.Init(procInfo, CoreTypes::ANY, (ULONG)2);
        printf("Initialized 'Any' Heterogeneous Threadpool (2 Threads)\n\r");

        // Reserve 1 thread for main thread & 1 thread for 'any' threadpool
        mCoreTaskScheduler.Init(procInfo, CoreTypes::INTEL_CORE, (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 2);
        printf("Initialized 'Core' Heterogeneous Threadpool (%d Threads)\n\r", (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 2);

        // Reserve 1 thread for 'any' threadpool
        mAtomTaskScheduler.Init(procInfo, CoreTypes::INTEL_ATOM, (ULONG)procInfo.cpuSets[CoreTypes::INTEL_ATOM].size() - 1);
        printf("Initialized 'Atom' Heterogeneous Threadpool (%d Threads)\n\r", (ULONG)procInfo.cpuSets[CoreTypes::INTEL_ATOM].size() - 1);
#else
        // Reserve 1 thread for main thread
		mCoreTaskScheduler.Init(procInfo, CoreTypes::INTEL_CORE, (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 1);
        printf("Initialized 'Core' Heterogeneous Threadpool (%d Threads)\n\r", (ULONG)procInfo.cpuSets[CoreTypes::INTEL_CORE].size() - 1);
		mAtomTaskScheduler.Init(procInfo, CoreTypes::INTEL_ATOM, (ULONG)procInfo.cpuSets[CoreTypes::INTEL_ATOM].size());
        printf("Initialized 'Atom' Heterogeneous Threadpool (%d Threads)\n\r", (ULONG)procInfo.cpuSets[CoreTypes::INTEL_ATOM].size());
#endif
#endif
    }
    else
    {
        printf("%s\n\r", procInfo.brandString);
        printf("Logical Cores %d\n\r", procInfo.numLogicalCores);
        mTaskScheduler.Init(procInfo, CoreTypes::ANY, procInfo.numLogicalCores - 1);
        printf("Initialized Homogeneous Threadpool (%d Threads)\n\r", procInfo.numLogicalCores - 1);
    }

    return TRUE;
}

VOID TaskMgrSS::Shutdown()
{
    //  
    //  Release any left-over tasksets
    for( UINT uSet = 0; uSet < MAX_TASKSETS; ++uSet )
    {
        if( mSets[ uSet ].mpFunc )
        {
            WaitForSet( uSet );   
        }
    }

    if (mProcInfo.hybrid)
    {
#if CORE_ONLY
        mCoreTaskScheduler.Shutdown();
#else
#if RESERVE_ANY
        mAnyTaskScheduler.Shutdown();
#endif
        mAtomTaskScheduler.Shutdown();
        mCoreTaskScheduler.Shutdown();
#endif
    }
    else
    {
        mTaskScheduler.Shutdown();
    }
}


BOOL TaskMgrSS::CreateTaskSet(TASKSETFUNC     pFunc,
                              VOID*           pArg,
                              UINT            uTaskCount,
                              TASKSETHANDLE*  pInDepends,
                              UINT            uInDepends,
                              OPTIONAL LPCSTR szSetName,
                              TASKSETHANDLE*  pOutHandle,
                              CoreTypes       coreType)
{
    TASKSETHANDLE           hSet;
    TASKSETHANDLE           hSetParent = TASKSETHANDLE_INVALID;
    TASKSETHANDLE*          pDepends = pInDepends;
    UINT                    uDepends = uInDepends;
    BOOL                    bResult = FALSE;


    //  Validate incomming parameters
    if( 0 == uTaskCount || NULL == pFunc )
    {
        return FALSE;
    }

    //
    //  Allocate and setup the internal taskset
    //
    hSet = AllocateTaskSet();

    mSets[ hSet ].muRefCount        = 2;
    mSets[ hSet ].muStartCount      = uDepends;
    mSets[ hSet ].mpvArg            = pArg;
    mSets[ hSet ].muSize            = uTaskCount;
    mSets[ hSet ].muCompletionCount = uTaskCount;
    mSets[ hSet ].muTaskId          = uTaskCount;
    mSets[ hSet ].mhTaskset         = hSet;
    mSets[ hSet ].mpFunc            = pFunc;
    mSets[ hSet ].mbCompleted       = FALSE;
    mSets[ hSet ].mCoreType         = mProcInfo.hybrid ? coreType : CoreTypes::ANY;
    //mSets[ hSet ].mhAssignedSlot    = TASKSETHANDLE_INVALID;

#ifdef PROFILEGPA
    //
    //  Track task name if profiling is enabled
    if( szSetName )
    {
        StringCbCopyA(
            mSets[ hSet ].mszSetName,
            sizeof( mSets[ hSet ].mszSetName ),
            szSetName );
    }
    else
    {
        StringCbCopyA(
            mSets[ hSet ].mszSetName,
            sizeof( mSets[ hSet ].mszSetName ),
            "Unnamed Task" );
    }
#else
    UNREFERENCED_PARAMETER( szSetName );
#endif // PROFILEGPA

    //
    //  Iterate over the dependency list and setup the successor
    //  pointers in each parent to point to this taskset.
    //
	if(uDepends == 0)
	{
        if (mProcInfo.hybrid)
        {
#if CORE_ONLY
            mCoreTaskScheduler.AddTaskSet(hSet, uTaskCount);
#else
            switch (mSets[hSet].mCoreType)
            {
            case CoreTypes::INTEL_ATOM:
                mAtomTaskScheduler.AddTaskSet(hSet, uTaskCount);
                break;
            case CoreTypes::INTEL_CORE:
                mCoreTaskScheduler.AddTaskSet(hSet, uTaskCount);
                break;
            case CoreTypes::ANY:
#if RESERVE_ANY
                mAnyTaskScheduler.AddTaskSet(hSet, uTaskCount);
#else
                mCoreTaskScheduler.AddTaskSet(hSet, uTaskCount);
#endif
                break;
            default:
                mCoreTaskScheduler.AddTaskSet(hSet, uTaskCount);
                break;
            }
#endif
        }
        else
        {
            mTaskScheduler.AddTaskSet(hSet, uTaskCount);
        }
	}
    else for( UINT uDepend = 0; uDepend < uDepends; ++uDepend )
    {
        TASKSETHANDLE       hDependsOn = pDepends[ uDepend ];

        if(hDependsOn == TASKSETHANDLE_INVALID)
            continue;

        TaskSet *pDependsOn = &mSets[ hDependsOn ];
        LONG     lPrevCompletion;

        //
        //  A taskset with a new successor is consider incomplete even if it
        //  already has completed.  This mechanism allows us tasksets that are
        //  already done to appear active and capable of spawning successors.
        //
        lPrevCompletion = _InterlockedExchangeAdd( (LONG*)&pDependsOn->muCompletionCount, 1 );

        pDependsOn->mSuccessorsLock.aquire();

        UINT uSuccessor;
        for( uSuccessor = 0; uSuccessor < MAX_SUCCESSORS; ++uSuccessor )
        {
            if( NULL == pDependsOn->Successors[ uSuccessor ] )
            {
                pDependsOn->Successors[ uSuccessor ] = &mSets[ hSet ];
                break;
            }
        }

        //
        //  If the successor list is full we have a problem.  The app
        //  needs to give us more space by increasing MAX_SUCCESSORS
        //
        if( uSuccessor == MAX_SUCCESSORS )
        {
            printf( "Too many successors for this task set.\nIncrease MAX_SUCCESSORS\n" );
            pDependsOn->mSuccessorsLock.release();
            goto Cleanup;
        }

        pDependsOn->mSuccessorsLock.release();

        //  
        //  Mark the set as completed for the successor adding operation.
        //
        CompleteTaskSet( hDependsOn );
    }

    //  Set output taskset handle
    *pOutHandle = hSet;

    bResult = TRUE;

Cleanup:

    return bResult;
}

VOID TaskMgrSS::ReleaseHandle( TASKSETHANDLE hSet )
{
    _InterlockedDecrement( (LONG*)&mSets[ hSet ].muRefCount );
}


VOID TaskMgrSS::ReleaseHandles( TASKSETHANDLE *phSet,UINT uSet )
{
    for( UINT uIdx = 0; uIdx < uSet; ++uIdx )
    {
        ReleaseHandle( phSet[ uIdx ] );
    }
}

VOID TaskMgrSS::WaitForSet( TASKSETHANDLE               hSet )
{
    //
    //  Yield the main thread to SS to get our taskset done faster!
    //  NOTE: tasks can only be waited on once.  After that they will
    //  deadlock if waited on again.
    if( !mSets[ hSet ].mbCompleted )
    {
        if (mProcInfo.hybrid)
        {
#if CORE_ONLY
            mCoreTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
#else
            switch (mSets[hSet].mCoreType)
            {
            case CoreTypes::INTEL_ATOM:
                mAtomTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
                break;
            case CoreTypes::INTEL_CORE:
                mCoreTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
                break;
            case CoreTypes::ANY:
#if RESERVE_ANY
                mAnyTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
#else
                mCoreTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
#endif
                break;
            default:
                mCoreTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
                break;
            }
#endif
        }
        else
        {
            mTaskScheduler.WaitForFlag(&mSets[hSet].mbCompleted);
        }
    }

}

BOOL
TaskMgrSS::IsSetComplete( TASKSETHANDLE hSet )
{
    return TRUE == mSets[ hSet ].mbCompleted;
}


TASKSETHANDLE TaskMgrSS::AllocateTaskSet()
{
      // This line is fine
    UINT                        uSet = muNextFreeSet;

    //
    //  Create a new task set and find a slot in the TaskMgrSS to put it in.
    //
    //  NOTE: if we have too many tasks pending we will spin on the slot.  If
    //  spinning occures, see TaskMgrCommon.h and increase MAX_TASKSETS
    //

    //
    //  NOTE: Allocating tasksets is not thread-safe due to allocation of the
    //  slot for the task pointer.  This can be easily made threadsafe with 
    //  an interlocked op on the muNextFreeSet variable and a spin on the slot.  
    //  It will cost a small amount of performance.
    //
    while( NULL != mSets[ uSet ].mpFunc && mSets[ uSet ].muRefCount != 0 )
    { 
        uSet = ( uSet + 1 ) % MAX_TASKSETS;
    }

    muNextFreeSet = ( uSet + 1 ) & (MAX_TASKSETS - 1);

    return (TASKSETHANDLE)uSet;
}

VOID TaskMgrSS::CompleteTaskSet( TASKSETHANDLE hSet )
{
    TaskSet*             pSet = &mSets[ hSet ];

    UINT uCount = _InterlockedDecrement( (LONG*)&pSet->muCompletionCount );

    if( 0 == uCount )
    {
        pSet->mbCompleted = TRUE;
        pSet->mpFunc = 0;
        //
        //  The task set has completed.  We need to look at the successors
        //  and signal them that this dependency of theirs has completed.
        //
        pSet->mSuccessorsLock.aquire();

        for( UINT uSuccessor = 0; uSuccessor < MAX_SUCCESSORS; ++uSuccessor )
        {
            TaskSet* pSuccessor = pSet->Successors[ uSuccessor ];

            //
            //  A signaled successor must be removed from the Successors list 
            //  before the mSuccessorsLock can be released.
            //
            pSet->Successors[ uSuccessor ] = NULL;
            
            if( NULL != pSuccessor ) 
            {
                UINT uStart;

                uStart = _InterlockedDecrement( (LONG*)&pSuccessor->muStartCount );

                //
                //  If the start count is 0 the successor has had all its 
                //  dependencies satisified and can be scheduled.
                //
                if( 0 == uStart )
                {
                    if (mProcInfo.hybrid)
                    {
#if CORE_ONLY
                        mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#else
                        switch (pSuccessor->mCoreType)
                        {
                        case CoreTypes::INTEL_ATOM:
                            mAtomTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                            break;
                        case CoreTypes::INTEL_CORE:
                            mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                            break;
                        case CoreTypes::ANY:
#if RESERVE_ANY
                            mAnyTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#else
                            mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
#endif
                            break;
                        default:
                            mCoreTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                            break;
                        }
#endif
                    }
                    else
                    {
                        mTaskScheduler.AddTaskSet(pSuccessor->mhTaskset, pSuccessor->muSize);
                    }
                }
            }
        }

        pSet->mSuccessorsLock.release();

        ReleaseHandle( hSet );
    }
}
