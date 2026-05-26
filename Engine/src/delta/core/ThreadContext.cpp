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
            virtualRunwayCursor += ADDR_SLICE_PER_THREAD;

            InitializeArena(ctx.pageCoordinator, ctx.transientArena, MemoryMap::VIRT_ZONE_TA_OFFSET, MemoryMap::VIRT_ZONE_TA_BASELINE);
            InitializeArena(ctx.pageCoordinator, ctx.componentPoolArena, MemoryMap::VIRT_ZONE_CPA_OFFSET, MemoryMap::VIRT_ZONE_CPA_BASELINE);
            InitializeArena(ctx.pageCoordinator, ctx.sceneArena, MemoryMap::VIRT_ZONE_SA_OFFSET, MemoryMap::VIRT_ZONE_SA_BASELINE);

            /*uint8_t* initialPageTarget = ctx.pageCoordinator.virtualAddressBase + ctx.pageCoordinator.commitedOffset;
            void* initialPageMemory = delta::platform::Memory_Commit(initialPageTarget, pageSize);
            assert(initialPageMemory != nullptr && "Failed to commit initial arena page!");

            if (!delta::platform::Memory_Lock(initialPageTarget, pageSize))
                std::cout << "[DeltaEngine-Warning] Failed to lock memory resource: Thread-Local Arena " << i << "\n";

            ctx.pageCoordinator.commitedOffset += pageSize;
            ctx.transientArena.backingMemory = reinterpret_cast<uint8_t*>(initialPageTarget);
            ctx.transientArena.capacity = pageSize;
            ctx.transientArena.offset = 0;*/

            delta::platform::Timer_Initialize(&ctx.perThreadTimer);
        }

        tl_CurrentThreadContext = &g_ThreadContexts[0];
    }

    void ThreadContext_Shutdown()
    {
        delta::platform::Memory_Release(g_ThreadContexts);
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
