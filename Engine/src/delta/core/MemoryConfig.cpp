#include "MemoryConfig.h"

#include <delta/platform/os.h>

namespace delta::core
{
    EngineMemoryConfig g_MemoryConfig{};

    void MemoryConfig_Initialize(size_t physicalRamInstalled, uint32_t maxEngineWorkers)
    {
        g_MemoryConfig.totalPhysicalRam = physicalRamInstalled;
        g_MemoryConfig.globalLockCeiling = (g_MemoryConfig.totalPhysicalRam / 10) * 7;
        g_MemoryConfig.threadSoftBaseline = g_MemoryConfig.globalLockCeiling / maxEngineWorkers;
    }

    void MemoryConfig_Shutdown()
    {
        g_MemoryConfig = {};
    }
}
