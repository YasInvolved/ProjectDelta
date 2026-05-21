#include "MemoryManager.h"

#define ALIGN(size, alignment) (size + (alignment - 1)) & ~(alignment - 1)

namespace delta::core
{
    static ThreadExecutionContext* g_ThreadContexts = nullptr;
    thread_local ThreadExecutionContext* tl_CurrentThreadContext = nullptr;

    void MemoryManager_Initialize(uint32_t workerCount, size_t pageSize)
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

        uint8_t* virtualRunwayCursor = masterPoolBase + alignedContextArraySize;
        for (uint32_t i = 0; i < totalThreads; i++)
        {
            ThreadExecutionContext& ctx = g_ThreadContexts[i];
            ctx.threadIx = i;
            ctx.threadId = 0; // to be set later

            ctx.pageCoordinator.virtualAddressBase = virtualRunwayCursor;
            ctx.pageCoordinator.commitedOffset = 0;
            ctx.pageCoordinator.reservedCapacity = ADDR_SLICE_PER_THREAD;

            virtualRunwayCursor += ADDR_SLICE_PER_THREAD;

            uint8_t* initialPageTarget = ctx.pageCoordinator.virtualAddressBase + ctx.pageCoordinator.commitedOffset;
            void* initialPageMemory = delta::platform::Memory_Commit(initialPageTarget, pageSize);
            assert(initialPageMemory != nullptr && "Failed to commit initial arena page!");

            ctx.pageCoordinator.commitedOffset += pageSize;
            ctx.transientArena.backingMemory = reinterpret_cast<uint8_t*>(initialPageTarget);
            ctx.transientArena.capacity = pageSize;
            ctx.transientArena.offset = 0;

            delta::platform::Timer_Initialize(&ctx.perThreadTimer);
        }

        tl_CurrentThreadContext = &g_ThreadContexts[0];
    }

    void MemoryManager_Shutdown()
    {
        delta::platform::Memory_Release(g_ThreadContexts);
    }

    void* EngineArena_Allocate(EngineArena* arena, size_t size, size_t alignment)
    {
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(arena->backingMemory) + arena->offset;
        uintptr_t alignedAddress = ALIGN(currentAddress, alignment);
        size_t padding = alignedAddress - currentAddress;
        size_t totalSpace = padding + size;

        if (totalSpace > arena->capacity)
        {
            // TODO: Reallocate
            return nullptr;
        }

        uint8_t* ptr = reinterpret_cast<uint8_t*>(alignedAddress);
        arena->offset += totalSpace;

        return ptr;
    }

    void EngineArena_Reset(EngineArena* arena)
    {
        arena->offset = 0;
    }
}
