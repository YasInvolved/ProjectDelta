#pragma once
#include <delta/platform/os_types.h>

namespace delta::platform
{
    // Timer API
    struct Timer_Internal;
    struct Timer
    {
        alignas(8) uint8_t opaqueData[32];
    };

    // Thread API
    DLT_DEFINE_HANDLE(Thread);

    struct ThreadCreateInfo
    {
        void (*fn)(void*);
        void* args;
    };

    // Sync API
    DLT_DEFINE_HANDLE(Semaphore);

    // Window API
    // Window defined in the public header
}
