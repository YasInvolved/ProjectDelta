#include "MemoryConfig.h"
#include "EngineTypes.h"
#include <delta/platform/os.h>
#include <iostream>

namespace delta::core
{
    EngineMemoryConfig g_MemoryConfig{};

    void MemoryConfig_Initialize(size_t physicalRamInstalled, size_t pageSize, uint32_t totalThreads)
    {
        g_MemoryConfig.totalPhysicalRam = physicalRamInstalled;
        g_MemoryConfig.maxAllowedPhysical = (physicalRamInstalled / 10) * 7; // 70% of total capacity

        // TODO: Change this. "Environment" has changed.
        // calculate soft baseline (ideal case, no stealing from the global pool)
        g_MemoryConfig.threadSoftBaseline = MemoryMap::VIRT_ZONE_TA_BASELINE;

        // formula optimized on paper to limit arithmetic that has to be done
        // precompute whats possible to save some runtime
        static constexpr size_t PRECOMPUTABLE_CONST = THREAD_EXECUTION_CONTEXT_SIZE + MemoryMap::VIRT_ZONE_TA_BASELINE + MemoryMap::Worker::VIRT_ZONE_BASELINE_SUM;
        size_t rawLockBudget = (totalThreads * PRECOMPUTABLE_CONST) + MemoryMap::Main::VIRT_ZONE_BASELINE_SUM - MemoryMap::Worker::VIRT_ZONE_BASELINE_SUM;

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
