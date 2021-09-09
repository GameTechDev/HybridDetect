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
    \file TaskMgrTbb.h

    TaskMgrTbb is a class that wraps the TBB library with a C-style handle and 
    callback mechanism for scheduling tasks to scale across any number of CPU
    cores.

    TaskMgrTbb is a singleton object and is already instantiated for the app as
    gTaskMgr.  The TaskMgrTbb is single threaded, meaning that tasksets can only
    be created from one thread.  With minor changes and some performance loss,
    multiple thread can create task sets (see AllocateTaskSet in TaskMgrTbb.cpp. 
    The app can control two knobs in the TaskMgrTbb class through MAX_SUCCESSORS 
    and MAX_TASKSETS defined below.

    MAX_SUCCESSORS is the maximum number of tasksets that can be have depended
    on another taskset.  For example if you have Tasksets A,B,C and both B and 
    C can run simultaniously and both depend on A to complete (so A->(B,C)) then
    A has two successors.  There is a small performance hit for large numbers
    of successors so the value should be set to something reasonably close to 
    the maximum the app will use. The default value is 5.

    MAX_TASKSETS is the max number of tasksets that can be live at one  time. 
    A taskset is live if it has a non-zero reference count.  Increasing the 
    number of tasksets that can be live increases the memory footprint of the 
    TaskMgrTbb object.  The default value is 255.
*/
#pragma once



#include <wtypes.h>



//  Callback type for tasks in the tasking TaskMgrTBB system
typedef VOID (*TASKSETFUNC )( VOID*,
                              INT,
                              UINT,
                              UINT );

//  Handle to a task set that can be used to express task set
//  dependecies and task set synchronization.
typedef UINT        TASKSETHANDLE;


/*! The TaskMgrTbb allows the user to schedule tasksets that run on top of
    TBB.  All TaskMgrTbb functions are NOT threadsafe.  TaskMgrTbb is 
    designed to be called only from the main thread.  Multi-threading is 
    achieved by creating TaskSets that execte on threads created internally
    by TBB.
*/
class DynamicTaskMgrBase
{
public:

    //  Init will setup the tasking system.  It must be called before
    //  any other functions on the TaskMgrTbb interface.
    virtual BOOL
        Init() = 0;

    //  Shutdown will stop the tasking system. Any outstanding tasks will
    //  be terminated and the threads used by TBB will be released.  It is
    //  up to the application to wait on any outstanding tasks before it
    //  calls shutdown.
    virtual VOID
        Shutdown() = 0;

    //  Creates a task set and provides a handle to allow the application
    //  CreateTaskSet can fail if, by adding this task to the successor lists
    //  of its dependecies the list exceeds MAX_SUCCESSORS.  To fix, increase
    //  MAX_SUCCESSORS.
    //
    //  NOTE: A tasket of size 1 is valid.  The most common case is to have 
    //  tasksets of >> 1 so the default tasking primitive is a taskset rather
    //  than a task.
    virtual BOOL
    CreateTaskSet(
        TASKSETFUNC                 pFunc,      //  Function pointer to the 
        //  Taskset callback function

        VOID*                       pArg,       //  App data pointer (can be NULL)

        UINT                        uTaskCount, //  Number of tasks to create 

        TASKSETHANDLE*              pDepends,   //  Array of TASKSETHANDLEs that 
        //  this taskset depends on.  The 
        //  taskset will not be scheduled
        //  until all tasksets in this list
        //  complete.

        UINT                        uDepends,   //  Count of the depends list

        OPTIONAL LPCSTR             szSetName,  //  [Optional] name of the taskset
        //  the name is used for profiling

        OUT TASKSETHANDLE*          pOutHandle  //  [Out] Handle to the new taskset
        ) = 0;

    //  All TASKSETHANDLE must be released when no longer referenced.  
    //  ReleaseHandle will release the Applications reference on the taskset.
    //  It should only be called once per handle returned from CreateTaskSet.
    virtual VOID
        ReleaseHandle( TASKSETHANDLE hSet        //  Taskset handle to release
                       ) = 0;

    //  All TASKSETHANDLE must be released when no longer referenced.  
    //  ReleaseHandles will release the Applications reference on the array
    //  of taskset handled specified.  It should only be called once per handle 
    //  returned from CreateTaskSet.
    virtual VOID
        ReleaseHandles( TASKSETHANDLE* phSet,      //  Taskset handle array to release
                        UINT uSet        //  count of taskset handle array
                        ) = 0;

    //  WaitForSet will yeild the main thread to the tasking system and return
    //  only when the taskset specified has completed execution.
    virtual VOID
        WaitForSet( TASKSETHANDLE hSet        // Taskset to wait for completion
                    ) = 0;
};

extern DynamicTaskMgrBase* g_pTaskMgr;
