#include <iostream>
#include "ThreadContext.h"
#include "MemoryConfig.h"

#define ALIGN(size, alignment) ((size + (alignment - 1)) & ~(alignment - 1))

namespace delta::core
{
    static ThreadExecutionContext* g_ThreadContexts = nullptr;
    thread_local ThreadExecutionContext* tl_CurrentThreadContext = nullptr;

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

    DLT_FORCE_INLINE static void InitializeArena(const ThreadPageCoordinator& pageCoord, ThreadArena& arena, size_t offset, size_t baseline)
    {
        uint8_t* pTarget = pageCoord.virtualAddressBase + offset;
        void* res = delta::platform::Memory_Commit(pTarget, baseline);
        assert(res != nullptr);

        // TODO: Lock

        arena.backingMemory = pTarget;
        arena.capacity = baseline;
        arena.offset = 0;
    }

    void ThreadContext_Initialize(uint32_t workerCount, size_t pageSize)
    {
        uint32_t totalThreads = workerCount + 1; // include main thread

        // assign 32Gb of address space per thread
        // master pool size is rounded up to page size
        constexpr size_t ADDR_SLICE_PER_THREAD = (1ull << 35);
        size_t contextArraySize = sizeof(ThreadExecutionContext) * totalThreads;
        size_t alignedContextArraySize = ALIGN(contextArraySize, pageSize);
        size_t masterPoolSize = (ADDR_SLICE_PER_THREAD * totalThreads) + alignedContextArraySize;

        uint8_t* masterPoolBase = reinterpret_cast<uint8_t*>(
            delta::platform::Memory_Reserve(masterPoolSize)
        );
        assert(masterPoolBase != nullptr && "Failed to reserve master pool");


        g_ThreadContexts = reinterpret_cast<ThreadExecutionContext*>(
            delta::platform::Memory_Commit(masterPoolBase, alignedContextArraySize)
        );
        assert(g_ThreadContexts != nullptr && "Failed to commit thread context array");

        if (!delta::platform::Memory_Lock(masterPoolBase, alignedContextArraySize))
            std::cout << "[DeltaEngine-Warning] Failed to lock memory resource: Master Thread Context Pool\n";

        uint8_t* virtualRunwayCursor = masterPoolBase + alignedContextArraySize;
        for (uint32_t i = 0; i < totalThreads; i++)
        {
            ThreadExecutionContext& ctx = g_ThreadContexts[i];
            ctx.threadIx = i;
            ctx.threadId = 0; // to be set later

            InitializePageCoordinator(ctx.pageCoordinator, pageSize, virtualRunwayCursor);
            InitializeQueue(ctx.pageCoordinator, ctx.taskQueue, MemoryMap::VIRT_ZONE_QUEUE_OFFSET, MemoryMap::VIRT_ZONE_QUEUE_SIZE);
            InitializeArena(ctx.pageCoordinator, ctx.transientArena, MemoryMap::VIRT_ZONE_TA_OFFSET, MemoryMap::VIRT_ZONE_TA_BASELINE);
            InitializeArena(ctx.pageCoordinator, ctx.componentPoolArena, MemoryMap::VIRT_ZONE_CPA_OFFSET, MemoryMap::VIRT_ZONE_CPA_BASELINE);
            InitializeArena(ctx.pageCoordinator, ctx.sceneArena, MemoryMap::VIRT_ZONE_SA_OFFSET, MemoryMap::VIRT_ZONE_SA_BASELINE);

            delta::platform::Timer_Initialize(&ctx.perThreadTimer);
            virtualRunwayCursor += ADDR_SLICE_PER_THREAD;
        }

        tl_CurrentThreadContext = &g_ThreadContexts[0];
    }

    void ThreadContext_Shutdown()
    {
        delta::platform::Memory_Release(g_ThreadContexts);
    }

    void TaskQueue_Push(TaskQueue* queue, task_t task, payload_t payload)
    {
        uint64_t b = queue->bottom.load(std::memory_order_relaxed);
        uint64_t t = queue->top.load(std::memory_order_acquire);

        if (b - t > queue->size)
        {
            // execute immadietely if the queue is full
            task(payload);
            return;
        }

        uint64_t ix = b & queue->mask;
        queue->tasks[ix] = task;
        queue->payloads[ix] = payload;

        std::atomic_thread_fence(std::memory_order_release);
        queue->bottom.store(b + 1, std::memory_order_relaxed);
    }

    bool TaskQueue_Pop(TaskQueue* queue, task_t* outTask, payload_t* outPayload)
    {
        uint64_t b = queue->bottom.load(std::memory_order_relaxed) - 1;
        queue->bottom.store(b, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_seq_cst);
        uint64_t t = queue->top.load(std::memory_order_relaxed);

        if (t > b)
        {
            queue->bottom.store(b + 1, std::memory_order_relaxed);
            return false;
        }

        uint64_t ix = b & queue->mask;
        *outTask = queue->tasks[ix];
        *outPayload = queue->payloads[ix];

        if (t == b)
        {
            uint64_t expectedTop = t;
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
        uint64_t t = queue->top.load(std::memory_order_acquire);

        std::atomic_thread_fence(std::memory_order_seq_cst);
        uint64_t b = queue->bottom.load(std::memory_order_acquire);

        if (t >= b)
            return false;

        uint64_t ix = t & queue->mask;
        *outTask = queue->tasks[ix];
        *outPayload = queue->payloads[ix];

        if (!queue->top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            return false;

        return true;
    }

    void* ThreadArena_Allocate(ThreadArena* arena, size_t size, size_t alignment)
    {
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(arena->backingMemory) + arena->offset;
        uintptr_t alignedAddress = ALIGN(currentAddress, alignment);
        size_t padding = alignedAddress - currentAddress;
        size_t totalSpace = padding + size;

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
