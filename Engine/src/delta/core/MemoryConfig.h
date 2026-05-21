#pragma once

namespace delta::core
{
    struct EngineMemoryConfig
    {
        size_t totalPhysicalRam;
        size_t globalLockCeiling;
        size_t threadSoftBaseline;
    };

    extern EngineMemoryConfig g_MemoryConfig;
    extern std::atomic<size_t> g_TotalLockedBytes;

    void MemoryConfig_Initialize(size_t physicalRamInstalled, uint32_t maxEngineWorkers);
    void MemoryConfig_Shutdown();
}
