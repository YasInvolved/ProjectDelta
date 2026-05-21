#include "MemoryConfig.h"

#include <delta/platform/os.h>

namespace delta::core
{
    EngineMemoryConfig g_MemoryConfig{};
    std::atomic<size_t> g_TotalLockedBytes{ 0 };

    void MemoryConfig_Initialize(uint32_t maxEngineWorkers)
    {
        auto memStatus = delta::platform::getMemoryStatus();
        g_MemoryConfig.totalPhysicalRam = memStatus.physicalInstalled;
        g_MemoryConfig.globalLockCeiling = (g_MemoryConfig.totalPhysicalRam / 10) * 7;
        g_MemoryConfig.threadSoftBaseline = g_MemoryConfig.globalLockCeiling / maxEngineWorkers;

        g_TotalLockedBytes.store(0, std::memory_order_relaxed);
    }

    void MemoryConfig_Shutdown()
    {
        g_MemoryConfig = {};
        g_TotalLockedBytes.store(0, std::memory_order_relaxed);
    }
}
