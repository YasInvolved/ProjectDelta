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
    struct BrandStringCall
    {
        int c1[4];
        int c2[4];
        int c3[4];

        static constexpr char UNSPECIFIED_VALUE[] = "(unspecified)";
    };

    void Initialize();
    void* ReserveMemory(size_t reservationSize);
    void* CommitMemory(void* mem, size_t commitSize);
    void DecommitMemory(void* mem, size_t decommitSize);
    void ReleaseMemory(void* mem);

    struct OSThreadHandle;
    struct OSSemaphoreHandle;

    struct ThreadCreationInfo
    {
        void (*entryPoint)(void*);
        void* userData;
        uint32_t coreAffinityMask;
        const char* debugName;
    };

    OSThreadHandle* CreateEngineThread(const ThreadCreationInfo& info);
    void DestroyEngineThread(OSThreadHandle* thread);
    void SetThreadAffinity(OSThreadHandle* thread, uint32_t mask);

    OSSemaphoreHandle* CreateEngineSemaphore(uint32_t initialCount);
    void WaitOnSemaphore(OSSemaphoreHandle* sem);
    void SignalSemaphore(OSSemaphoreHandle* sem, uint32_t count);
}
