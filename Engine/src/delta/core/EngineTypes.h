#pragma once

namespace delta::core
{
    struct ThreadPageCoordinator
    {
        uint8_t* virtualAddressBase;
        size_t commitedOffset;
        size_t reservedCapacity;
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
}
