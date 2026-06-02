/*
 * Copyright 2026 Jakub Bączyk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <delta/platform/os_internal_types.h>

namespace delta::platform
{
    // Initialization
    struct BrandStringCall;
    void Initialize();

    // Memory API
    void*   Memory_Reserve(size_t reservationSize);
    void*   Memory_Commit(void* mem, size_t commitSize);
    void    Memory_Decommit(void* mem, size_t decommitSize);
    void    Memory_Release(void* mem);
    bool    Memory_Lock(void* mem, size_t bytes);
    bool    Memory_ElevateLockLimit(size_t maxBytesToLock);

    // Timer API
    void    Timer_Initialize(Timer* timer);
    int64_t Timer_GetTimestamp();
    double  Timer_TicksToMilliseconds(const Timer* timer, int64_t startTicks, int64_t endTicks);
    double  Timer_TicksToMicroseconds(const Timer* timer, int64_t startTicks, int64_t endTicks);

    // Thread API
    uint32_t Thread_GetCurrentId();
    uint32_t Thread_GetId(ThreadHandle thread);
    ThreadHandle Thread_GetCurrentHandle();
    ThreadHandle Thread_Create(ThreadCreateInfo* createInfo);
    void Thread_AssignPhysicalCore(ThreadHandle thread, uint32_t coreIndex);
    void Thread_SetName(ThreadHandle handle, const char* name);
    void Thread_Join(ThreadHandle thread);
    void Thread_JoinMultiple(ThreadHandle* threads, uint32_t count);

    // Sync API
    SemaphoreHandle Sync_CreateSemaphore();
    void Sync_DestroySemaphore(SemaphoreHandle sem);
    void Sync_SignalSemaphore(SemaphoreHandle sem);
    void Sync_WaitSemaphore(SemaphoreHandle sem);
    void Sync_Sleep(uint32_t milliseconds);

    // Window API
    WindowHandle Window_Create();
    void Window_Win32_Update(WindowHandle window);
    void Window_ProcessEvents();
    void Window_Destroy(WindowHandle window);
}
