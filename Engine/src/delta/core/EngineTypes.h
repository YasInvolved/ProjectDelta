#pragma once

#include <delta/platform/os_internal.h>

namespace delta::core
{
    // TODO: revise this structure and its purpose
    struct ThreadPageCoordinator
    {
        uint8_t* virtualAddressBase;
        size_t commitedOffset;
        size_t reservedCapacity;
        size_t pageSize;
    };

    struct ThreadArena
    {
        uint8_t* backingMemory;
        size_t capacity;
        size_t offset;
    };

    using task_t = void (*)(void*);
    struct TaskQueue // SoA structure, Chase-Lev queue
    {
        alignas(64) std::atomic<uint64_t> top;
        alignas(64) std::atomic<uint64_t> bottom;

        alignas(64) uint64_t size;
        uint64_t mask;

        task_t* tasks;
        void** payloads;

        static inline constexpr size_t FIELD_SIZE = sizeof(decltype(tasks)) + sizeof(decltype(payloads));
    };

    struct alignas(64) ThreadExecutionContext
    {
        uint32_t threadIx;
        uint32_t threadId;

        ThreadPageCoordinator pageCoordinator;
        TaskQueue taskQueue;

        alignas(64) ThreadArena transientArena;
        ThreadArena componentPoolArena;
        ThreadArena sceneArena;

        alignas(64) delta::platform::Timer perThreadTimer;

        uint8_t hardwarePadding[32];
    };

    extern thread_local ThreadExecutionContext* tl_CurrentThreadContext;

    static_assert((sizeof(ThreadExecutionContext) % 64) == 0);
}
