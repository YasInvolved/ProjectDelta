#pragma once

#include "EngineTypes.h"

namespace delta::core
{
    // Lifecycle
    void ThreadContext_Initialize(uint32_t workerCount, size_t pageSize);
    void ThreadContext_Shutdown();

    // Engine Arena API
    void*   ThreadArena_Allocate(EngineArena* arena, size_t size, size_t alignment = 8);
    void    ThreadArena_Reset(EngineArena* arena);
}
