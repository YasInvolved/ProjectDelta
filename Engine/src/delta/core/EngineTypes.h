#pragma once

#include <delta/platform/os_internal.h>

namespace delta::core
{
    struct ThreadPageCoordinator
    {
        uint8_t* virtualAddressBase;
        size_t commitedOffset;
        size_t reservedCapacity;
        size_t pageSize;
    };

    struct EngineArena
    {
        uint8_t* backingMemory;
        size_t capacity;
        size_t offset;
    };

    struct alignas(64) ThreadExecutionContext
    {
        uint32_t threadIx;
        uint32_t threadId;

        ThreadPageCoordinator pageCoordinator;
        EngineArena transientArena;
        delta::platform::Timer perThreadTimer;
    };

    extern thread_local ThreadExecutionContext* tl_CurrentThreadContext;
}
