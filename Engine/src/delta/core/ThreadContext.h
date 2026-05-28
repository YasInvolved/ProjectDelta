#pragma once

#include "EngineTypes.h"

namespace delta::core
{
    // Lifecycle
    void ThreadContext_Initialize(uint32_t workerCount, size_t pageSize);
    void ThreadContext_Shutdown();

    // Thread Task Queue API
    void TaskQueue_Push(TaskQueue* queue, task_t task, payload_t payload);
    bool TaskQueue_Pop(TaskQueue* queue, task_t* outTask, payload_t* outPayload);
    bool TaskQueue_Steal(TaskQueue* queue, task_t* outTask, payload_t* outPayload);

    // Scheduler API
    void Scheduler_ProcessTaskBatch(task_t* tasks, payload_t* payloads, size_t length);
    void Scheduler_Sync();

    // Engine Arena API
    void*   ThreadArena_Allocate(ThreadArena* arena, size_t size, size_t alignment = 8);
    void    ThreadArena_Reset(ThreadArena* arena);

    //template <typename T>
    //inline T* ThreadArena_AllocateForType(ThreadArena* arena, size_t alignment = 8)
    //{
    //    return reinterpret_cast<T*>(ThreadArena_Allocate(arena, sizeof(T), alignment);
    //}
}
