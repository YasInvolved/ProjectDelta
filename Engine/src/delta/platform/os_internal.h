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

namespace delta::platform
{
    struct BrandStringCall;
    struct Timer_Internal;

    struct ThreadCreateInfo
    {
        void (*fn)(void*);
        void* args;
    };

    struct Thread;
    using ThreadHandle = Thread*;

    struct Semaphore;
    using SemaphoreHandle = Semaphore*;

    struct Timer
    {
        alignas(8) uint8_t opaqueData[32];
    };

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
    ThreadHandle Thread_Create(ThreadCreateInfo* createInfo);
    void Thread_Join(ThreadHandle thread);
    void Thread_JoinMultiple(ThreadHandle* threads, uint32_t count);

    // Sync API
    SemaphoreHandle Sync_CreateSemaphore();
    void Sync_DestroySemaphore(SemaphoreHandle sem);
    void Sync_SignalSemaphore(SemaphoreHandle sem);
    void Sync_WaitSemaphore(SemaphoreHandle sem);
}
