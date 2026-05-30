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

#include <iostream>
#include "ThreadContext.h"
#include "MemoryConfig.h"

#define ALIGN(size, alignment) ((size + (alignment - 1)) & ~(alignment - 1))

namespace delta::core
{
    uintptr_t g_MasterSlabStart = 0;
    uintptr_t g_MasterSlabEnd = 0;

    static uint32_t g_ThreadCount = 0;
    static uint32_t g_WorkerCount = 0;
    static GenericExecutionContext* g_ThreadContexts = nullptr;
    static thread_local GenericExecutionContext* tl_CurrentThreadContext = nullptr;

    DLT_FORCE_INLINE static void InitializePageCoordinator(ThreadPageCoordinator& pageCoord, size_t pageSize, uint8_t* baseAddress)
    {
        pageCoord.pageSize = pageSize;
        pageCoord.virtualAddressBase = baseAddress;
        pageCoord.commitedOffset = 0;
        pageCoord.reservedCapacity = MemoryMap::VIRT_ZONE_SPACE_LENGTH;
    }

    DLT_FORCE_INLINE static void InitializeQueue(const ThreadPageCoordinator& pageCoord, TaskQueue& queue, size_t offset, size_t memSize)
    {
        queue.size = memSize / TaskQueue::FIELD_SIZE;
        queue.mask = queue.size - 1;
        queue.top.store(0, std::memory_order_relaxed);
        queue.bottom.store(0, std::memory_order_relaxed);

        // a queue is commited in whole
        uint8_t* pTarget = pageCoord.virtualAddressBase + offset;
        void* p = delta::platform::Memory_Commit(pTarget, memSize);
        assert(p != nullptr);

        uint8_t* tasksArrayPtr = pTarget;
        uint8_t* payloadsArrayPtr = pTarget + queue.size;
        queue.tasks = reinterpret_cast<task_t*>(tasksArrayPtr);
        queue.payloads = reinterpret_cast<void**>(payloadsArrayPtr);
    }

    DLT_FORCE_INLINE static void InitializeArena(const ThreadPageCoordinator& pageCoord, ThreadArena& arena, size_t offset, size_t baseline, size_t maxCapacity)
    {
        uint8_t* pTarget = pageCoord.virtualAddressBase + offset;
        void* res = delta::platform::Memory_Commit(pTarget, baseline);
        assert(res != nullptr);

        // TODO: Lock

        arena.backingMemory = pTarget;
        arena.capacity = baseline;
        arena.offset = 0;
        arena.maxCapacity = maxCapacity;
    }

    template <ExecutionContext ContextType>
    DLT_FORCE_INLINE ContextType& GetExecutionContext(size_t i)
    {
        return reinterpret_cast<ContextType&>(*(reinterpret_cast<uint8_t*>(g_ThreadContexts) + i * THREAD_EXECUTION_CONTEXT_SIZE));
    }

    void ThreadContext_Initialize(uint32_t threadCount, size_t pageSize)
    {
        g_ThreadCount = threadCount;
        g_WorkerCount = threadCount - 1;

        // assign 32Gb of address space per thread
        // master pool size is rounded up to page size
        constexpr size_t ADDR_SLICE_PER_THREAD = (1ull << 35);
        size_t contextArraySize = THREAD_EXECUTION_CONTEXT_SIZE * threadCount;
        size_t alignedContextArraySize = ALIGN(contextArraySize, pageSize);
        size_t masterPoolSize = (ADDR_SLICE_PER_THREAD * threadCount) + alignedContextArraySize;

        uint8_t* masterPoolBase = reinterpret_cast<uint8_t*>(
            delta::platform::Memory_Reserve(masterPoolSize)
        );
        assert(masterPoolBase != nullptr && "Failed to reserve master pool");

        g_MasterSlabStart = reinterpret_cast<uintptr_t>(masterPoolBase);
        g_MasterSlabEnd = g_MasterSlabStart + masterPoolSize;

        g_ThreadContexts = reinterpret_cast<GenericExecutionContext*>(
            delta::platform::Memory_Commit(masterPoolBase, alignedContextArraySize)
        );
        assert(g_ThreadContexts != nullptr && "Failed to commit thread context array");

        if (!delta::platform::Memory_Lock(masterPoolBase, alignedContextArraySize))
            std::cout << "[DeltaEngine-Warning] Failed to lock memory resource: Master Thread Context Pool\n";

        // pre-initialize generics
        uint8_t* runwayCursor = masterPoolBase + alignedContextArraySize;
        for (uint32_t i = 0; i < threadCount; i++)
        {
            GenericExecutionContext& ctx = GetExecutionContext<GenericExecutionContext&>(i);
            InitializePageCoordinator(ctx.pageCoordinator, pageSize, runwayCursor);
            InitializeArena(ctx.pageCoordinator, ctx.transientArena, MemoryMap::VIRT_ZONE_TA_OFFSET, MemoryMap::VIRT_ZONE_TA_BASELINE, MemoryMap::VIRT_ZONE_TA_SIZE);
            delta::platform::Timer_Initialize(&ctx.perThreadTimer);
            runwayCursor += MemoryMap::VIRT_ZONE_SPACE_LENGTH;
        }

        // Finish initializing main thread context
        {
            MainExecutionContext& ctx = GetExecutionContext<MainExecutionContext&>(0);
            ctx.generic.type = ThreadType::MAIN;
            ctx.generic.threadIx = 0;
            ctx.generic.threadId = delta::platform::Thread_GetCurrentId();

            InitializeArena(ctx.generic.pageCoordinator, ctx.persistentStorage, MemoryMap::Main::VIRT_ZONE_PS_OFFSET, MemoryMap::Main::VIRT_ZONE_PS_BASELINE, MemoryMap::Main::VIRT_ZONE_PS_SIZE);
        }

        for (uint32_t i = 1; i < threadCount; i++)
        {
            WorkerExecutionContext& ctx = GetExecutionContext<WorkerExecutionContext>(i);
            ctx.generic.type = ThreadType::WORKER;
            ctx.generic.threadIx = i;
            ctx.generic.threadId = 0xDEADBEEFu; // Initialized when thread starts
            ctx.isAsleep.store(false, std::memory_order_relaxed);
            ctx.shouldClose.store(false, std::memory_order_relaxed);

            InitializeQueue(ctx.generic.pageCoordinator, ctx.taskQueue, MemoryMap::Worker::VIRT_ZONE_QUEUE_OFFSET, MemoryMap::Worker::VIRT_ZONE_QUEUE_SIZE);
        }

        tl_CurrentThreadContext = &g_ThreadContexts[0];
    }

    void ThreadContext_Shutdown()
    {
        delta::platform::Memory_Release(g_ThreadContexts);
    }

    GenericExecutionContext* ThreadContext_GetCurrent() noexcept
    {
        return tl_CurrentThreadContext;
    }

    GenericExecutionContext* ThreadContext_GetForIndex(uint32_t i) noexcept
    {
        return &GetExecutionContext<GenericExecutionContext>(i);
    }

    void ThreadContext_SetCurrent(GenericExecutionContext* ctx) noexcept
    {
        tl_CurrentThreadContext = ctx;
    }

    void TaskQueue_Push(TaskQueue* queue, task_t task, payload_t payload)
    {
        queue_index_t b = queue->bottom.load(std::memory_order_relaxed);
        queue_index_t t = queue->top.load(std::memory_order_acquire);

        if (b - t > queue->size)
        {
            // execute immadietely if the queue is full
            task(payload);
            return;
        }

        queue_index_t ix = b & queue->mask;
        queue->tasks[ix] = task;
        queue->payloads[ix] = payload;

        std::atomic_thread_fence(std::memory_order_release);
        queue->bottom.store(b + 1, std::memory_order_relaxed);
    }

    bool TaskQueue_Pop(TaskQueue* queue, task_t* outTask, payload_t* outPayload)
    {
        queue_index_t b = queue->bottom.load(std::memory_order_relaxed) - 1;
        queue->bottom.store(b, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_seq_cst);
        queue_index_t t = queue->top.load(std::memory_order_relaxed);

        if (t > b)
        {
            queue->bottom.store(b + 1, std::memory_order_relaxed);
            return false;
        }

        queue_index_t ix = b & queue->mask;
        *outTask = queue->tasks[ix];
        *outPayload = queue->payloads[ix];

        if (t == b)
        {
            queue_index_t expectedTop = t;
            if (!queue->top.compare_exchange_strong(expectedTop, expectedTop + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                queue->bottom.store(b + 1, std::memory_order_relaxed);
                return false;
            }

            queue->bottom.store(b + 1, std::memory_order_relaxed);
        }

        return true;
    }

    bool TaskQueue_Steal(TaskQueue* queue, task_t* outTask, payload_t* outPayload)
    {
        queue_index_t t = queue->top.load(std::memory_order_acquire);

        std::atomic_thread_fence(std::memory_order_seq_cst);
        queue_index_t b = queue->bottom.load(std::memory_order_acquire);

        if (t >= b)
            return false;

        queue_index_t ix = t & queue->mask;
        *outTask = queue->tasks[ix];
        *outPayload = queue->payloads[ix];

        if (!queue->top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            return false;

        return true;
    }

    void Scheduler_ProcessTaskBatch(task_t* tasks, payload_t* payloads, size_t length)
    {
        assert(IsMainThread()); // CAN BE EXECUTED ONLY ON MAIN THREAD!

        for (size_t i = 0; i < length; i++)
        {
            size_t targetIx = 1 + (i % g_WorkerCount);
            WorkerExecutionContext& ctx = GetExecutionContext<WorkerExecutionContext>(targetIx);
            TaskQueue_Push(&ctx.taskQueue, tasks[i], payloads[i]);

            if (ctx.isAsleep.load(std::memory_order_acquire))
                delta::platform::Sync_SignalSemaphore(ctx.sleepSemaphore);
        }
    }

    void Scheduler_Sync()
    {
        assert(tl_CurrentThreadContext->threadIx == 0); // CAN BE EXECUTED ONLY ON MAIN THREAD!

        uint32_t workerIx = 1;
        uint32_t consecutiveEmptyQueues = 0;

        while (consecutiveEmptyQueues < g_ThreadCount)
        {
            WorkerExecutionContext& ctx = GetExecutionContext<WorkerExecutionContext>(workerIx);
            task_t task = nullptr;
            payload_t payload = nullptr;

            if (TaskQueue_Steal(&ctx.taskQueue, &task, &payload))
            {
                consecutiveEmptyQueues = 0;
                assert(task && payload);

                size_t snapshot = tl_CurrentThreadContext->transientArena.offset;
                task(payload);
                tl_CurrentThreadContext->transientArena.offset = snapshot;
            }
            else
            {
                consecutiveEmptyQueues++;
            }

            workerIx = 1 + (workerIx % g_WorkerCount);
        }
    }

    void* ThreadArena_Allocate(ThreadArena* arena, size_t size, size_t alignment)
    {
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(arena->backingMemory) + arena->offset;
        uintptr_t alignedAddress = ALIGN(currentAddress, alignment);
        size_t padding = alignedAddress - currentAddress;
        size_t totalSpace = padding + size;

        if (arena->offset + totalSpace > arena->maxCapacity)
        {
            // TODO: Handle it better. I don't know how yet, but I will know soon.
            assert(false); // arena overflown
        }

        if (totalSpace > arena->capacity)
        {
            // the slow path: assign more pages
            // assigning twice 2 up front to avoid further slower-path-taking
            size_t bytesNeeded = totalSpace - arena->capacity;
            size_t spaceInPages = ALIGN(bytesNeeded, tl_CurrentThreadContext->pageCoordinator.pageSize) * 2;
            uint8_t* targetAddress = arena->backingMemory + arena->capacity;
            void* p = delta::platform::Memory_Commit(targetAddress, spaceInPages);
            if (p && delta::platform::Memory_Lock(p, spaceInPages))
            {
                return nullptr;
            }

            arena->capacity += spaceInPages;
        }

        uint8_t* ptr = reinterpret_cast<uint8_t*>(alignedAddress);
        arena->offset += totalSpace;

        return ptr;
    }

    void ThreadArena_Reset(ThreadArena* arena)
    {
        arena->offset = 0;
    }
}
