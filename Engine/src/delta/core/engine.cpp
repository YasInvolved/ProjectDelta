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

#include <delta/core/engine.h>
#include <delta/core/memory.h>
#include <delta/platform/os.h>
#include <delta/platform/os_internal.h>

#include "EngineTypes.h"
#include "MemoryConfig.h"
#include "ThreadContext.h"
#include "Worker.h"

namespace delta::Engine
{
    using GenericExecutionContext = delta::core::GenericExecutionContext;

    void Initialize(Context& context)
    {
        context.isRunning = true;
        delta::platform::Initialize();
        const auto* osInfo = delta::platform::getOSInfo();
        const auto memStatus = delta::platform::getMemoryStatus();

        uint32_t totalThreads = osInfo->cpuPhysicalCoreCount;
        uint32_t pageSize = osInfo->osPageSize;
        delta::core::MemoryConfig_Initialize(memStatus.physicalInstalled, pageSize, totalThreads);
        delta::core::ThreadContext_Initialize(totalThreads, pageSize);
        delta::core::Worker_Init(totalThreads-1);
    }

    void Update(Context& context)
    {
        // blah blah blah
        // do something
        delta::platform::Sync_Sleep(100);
        delta::core::ThreadArena_Reset(delta::core::GetTransientArena());
    }

    void Shutdown(Context& context)
    {
        delta::core::Worker_Shutdown();
        delta::core::ThreadContext_Shutdown();
        delta::core::MemoryConfig_Shutdown();
    }

    [[nodiscard]] void* Allocate(size_t size, AllocationType type, size_t alignment) noexcept
    {
        GenericExecutionContext* threadContext = delta::core::ThreadContext_GetCurrent();

        if (!threadContext)
        {
            return ::malloc(size);
        }

        if (type == AllocationType::TRANSIENT)
        {
            return core::ThreadArena_Allocate(&threadContext->transientArena, size, alignment);
        }
        else if (type == AllocationType::PERSISTENT && core::IsMainThread())
        {
            core::MainExecutionContext* ctx = reinterpret_cast<core::MainExecutionContext*>(threadContext);
            return core::ThreadArena_Allocate(&ctx->persistentStorage, size, alignment);
        }

        assert(false); // CRITICAL FAILURE: Workers cannot allocate persistent memory!
        return nullptr;
    }

    void Free(void* ptr) noexcept
    {
        if (!core::IsCustomAllocated(ptr))
            ::free(ptr);
    }
}
