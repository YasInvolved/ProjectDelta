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

#include "EngineTypes.h"

namespace delta::core
{
    // Lifecycle
    void ThreadContext_Initialize(uint32_t workerCount, size_t pageSize);
    void ThreadContext_Shutdown();

    // Getters
    GenericExecutionContext* ThreadContext_GetCurrent() noexcept;
    GenericExecutionContext* ThreadContext_GetForIndex(uint32_t ix) noexcept;
    void ThreadContext_SetCurrent(GenericExecutionContext* ctx) noexcept;

    // Thread Task Queue API
    void TaskQueue_Push(TaskQueue* queue, task_t task, payload_t payload);
    bool TaskQueue_Pop(TaskQueue* queue, task_t* outTask, payload_t* outPayload);
    bool TaskQueue_Steal(TaskQueue* queue, task_t* outTask, payload_t* outPayload);

    // Scheduler API
    void Scheduler_ProcessTaskBatch(task_t* tasks, payload_t* payloads, size_t length);
    void Scheduler_Sync();

    // Engine Arena API
    ThreadArena*    GetTransientArena() noexcept;
    void*           ThreadArena_Allocate(ThreadArena* arena, size_t size, size_t alignment = 8);
    void            ThreadArena_Reset(ThreadArena* arena);
    void            ThreadArena_Reset(ThreadArena* arena);

    // Inline Helpers
    template <ExecutionContext TargetType>
    [[nodiscard]] inline TargetType* ThreadContextCast(GenericExecutionContext* ctx)
    {
        if (!ctx) return nullptr;

        if constexpr (std::is_same_v<TargetType, MainExecutionContext>)
        {
            if (ctx->type == ThreadType::MAIN)
                return reinterpret_cast<TargetType*>(ctx);
        }
        else if constexpr (std::is_same_v<TargetType, WorkerExecutionContext>)
        {
            if (ctx->type == ThreadType::WORKER)
                return reinterpret_cast<TargetType*>(ctx);
        }

        assert(false); // CRITICAL: Casting to invalid type
        return nullptr;
    }

    inline bool IsCustomAllocated(void* ptr)
    {
        // range scan
        return g_MasterSlabStart <= reinterpret_cast<uintptr_t>(ptr) && reinterpret_cast<uintptr_t>(ptr) <= g_MasterSlabEnd;
    }

    inline bool IsMainThread()
    {
        auto* ctx = ThreadContext_GetCurrent();
        return ctx && ctx->type == ThreadType::MAIN;
    }

    inline bool IsWorkerThread()
    {
        auto* ctx = ThreadContext_GetCurrent();
        return ctx && ctx->type == ThreadType::WORKER;
    }
}
