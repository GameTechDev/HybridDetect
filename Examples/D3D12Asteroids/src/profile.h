///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

void ProfileBeginFrame(size_t frameIndex);
void ProfileEndFrame();

void ProfileBeginRender();
void ProfileEndRender();

void ProfileBeginRenderSubmit();
void ProfileEndRenderSubmit();

void ProfileBeginFenceWait();
void ProfileEndFenceWait();

void ProfileBeginPresent(size_t backBufferIndex = 0);
void ProfileEndPresent();

void ProfileBeginRenderSubset();
void ProfileEndRenderSubset();

void ProfileBeginSimUpdate();
void ProfileEndSimUpdate();

void ProfileBeginUpdate();
void ProfileEndUpdate();

void ProfileBeginFrameLockWait();
void ProfileEndFrameLockWait();

#define ProfileBeginTask( name )    __itt_task_begin( gDomain, __itt_null, __itt_null, __itt_string_handle_createA( name ) )
#define ProfileEndTask()            __itt_task_end( gDomain )