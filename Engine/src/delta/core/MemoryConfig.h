#pragma once

namespace delta::core
{
    // TODO: This is actually crap. In this file, there should be a tight, flat memory map for each thread
    // possibly modificable via config macros or maybe slightly dynamic.

    struct EngineMemoryConfig
    {
        size_t totalPhysicalRam;
        size_t globalLockCeiling;
        size_t threadSoftBaseline;
    };

    extern EngineMemoryConfig g_MemoryConfig;
    extern std::atomic<size_t> g_TotalLockedBytes; // TODO: Remove, redesign. completely unnecessary

    void MemoryConfig_Initialize(size_t physicalRamInstalled, uint32_t maxEngineWorkers);
    void MemoryConfig_Shutdown();
}
