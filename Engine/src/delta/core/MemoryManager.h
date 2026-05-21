#pragma once

#include "EngineTypes.h"

namespace delta::core
{
    // Lifecycle
    void MemoryManager_Initialize(uint32_t workerCount, size_t pageSize);
    void MemoryManager_Shutdown();

    // Engine Arena API
    void*   EngineArena_Allocate(EngineArena* arena, size_t size, size_t alignment = 8);
    void    EngineArena_Reset(EngineArena* arena);
}
