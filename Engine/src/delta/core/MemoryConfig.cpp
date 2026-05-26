#include "MemoryConfig.h"
#include "EngineTypes.h"
#include <delta/platform/os.h>
#include <iostream>

namespace delta::core
{
    EngineMemoryConfig g_MemoryConfig{};

    void MemoryConfig_Initialize(size_t physicalRamInstalled, size_t pageSize, uint32_t maxEngineWorkers)
    {
        g_MemoryConfig.totalPhysicalRam = physicalRamInstalled;
        g_MemoryConfig.maxAllowedPhysical = (physicalRamInstalled / 10) * 7; // 70% of total capacity

        // calculate soft baseline (ideal case, no stealing from the global pool)
        g_MemoryConfig.threadSoftBaseline = MemoryMap::VIRT_ZONE_TA_BASELINE;

        size_t totalControlTrackBytes = maxEngineWorkers * (sizeof(ThreadExecutionContext) + MemoryMap::VIRT_ZONE_QUEUE_SIZE);
        size_t totalPrivateArenaBytes = maxEngineWorkers * MemoryMap::VIRT_ZONE_BASELINE_SUM;
        size_t rawLockBudget = totalControlTrackBytes + totalPrivateArenaBytes;

        g_MemoryConfig.activeLockAllocation = (rawLockBudget + (pageSize - 1)) & ~(pageSize - 1);
        g_MemoryConfig.globalPoolSize = g_MemoryConfig.maxAllowedPhysical - g_MemoryConfig.activeLockAllocation;
        assert(g_MemoryConfig.activeLockAllocation <= g_MemoryConfig.globalPoolSize);

        bool result = delta::platform::Memory_ElevateLockLimit(g_MemoryConfig.activeLockAllocation);
        if (!result)
            std::cout << "[DeltaEngine-Warning] Failed to elevate process working set quota\n";
    }

    void MemoryConfig_Shutdown()
    {
        g_MemoryConfig = {};
    }
}
