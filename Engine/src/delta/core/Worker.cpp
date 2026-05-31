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

#include "Worker.h"
#include <delta/platform/os_internal.h>
#include <delta/core/ThreadContext.h>

namespace delta::core
{
    using ThreadHandle = delta::platform::ThreadHandle;
    using ThreadCreateInfo = delta::platform::ThreadCreateInfo;

    using SemaphoreHandle = delta::platform::SemaphoreHandle;

    static uint32_t s_WorkerCount;
    static ThreadHandle* s_Threads;

    static void WorkerProc(void* args)
    {
        GenericExecutionContext* generic = reinterpret_cast<GenericExecutionContext*>(args);
        ThreadContext_SetCurrent(generic);

        WorkerExecutionContext& ctx = *ThreadContextCast<WorkerExecutionContext>(generic);

        task_t task;
        payload_t payload;
        while (!ctx.shouldClose.load(std::memory_order_acquire))
        {
            // TODO: Figure out how to select a worker to steal some work from.
            // I'll probably utilize modulo n cyclic groups. (algebra - group theory)

            if (TaskQueue_Pop(&ctx.taskQueue, &task, &payload))
            {
                task(payload);
                continue;
            }

            ThreadArena_Reset(GetTransientArena());
            ctx.isAsleep.store(true, std::memory_order_release);
            if (!ctx.shouldClose.load(std::memory_order_acquire))
            {
                delta::platform::Sync_WaitSemaphore(ctx.sleepSemaphore);
            }

            ctx.isAsleep.store(false, std::memory_order_release);
        }
    }
    
    void Worker_Init(uint32_t count)
    {
        s_WorkerCount = count;

        // TODO: Take a look at native thread api and figure out how this could be done better
        // TODO: Set thread affinity mask
        s_Threads = new(delta::Engine::AllocationType::PERSISTENT) ThreadHandle[count];
        ThreadCreateInfo* cs = new(delta::Engine::AllocationType::TRANSIENT) ThreadCreateInfo[count];

        for (uint32_t i = 0; i < count; i++)
        {
            // looping through thread indices, workers start from ix=1
            ThreadCreateInfo& info = cs[i];
            ThreadHandle& handle = s_Threads[i];
            auto* ctx = ThreadContext_GetForIndex(i + 1);
            info.fn = WorkerProc;
            info.args = (void*)ctx;
            handle = delta::platform::Thread_Create(&info);
            ctx->threadHandle = handle;
            delta::platform::Thread_AssignPhysicalCore(handle, i + 1);
            delta::platform::Thread_SetName(handle, "Worker");
        }
    }

    void Worker_Shutdown()
    {
        delta::platform::Thread_JoinMultiple(s_Threads, s_WorkerCount);
    }
}
