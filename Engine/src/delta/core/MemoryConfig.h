#pragma once

namespace delta::core
{
    // FUTURE TODO: Make it configurable via compile definitions
    namespace MemoryMap
    {
        inline constexpr size_t VIRT_ZONE_SPACE_LENGTH = (1ull << 35);

        // COMPONENTS:
        // QUEUE
        inline constexpr size_t VIRT_ZONE_QUEUE_OFFSET = 0ull;
        inline constexpr size_t VIRT_ZONE_QUEUE_SIZE = (1ull << 16); // 64KB
        inline constexpr size_t VIRT_ZONE_QUEUE_BASELINE = UINT64_MAX; // No baseline, we commit it all

        // TRANSIENT ARENA
        inline constexpr size_t VIRT_ZONE_TA_OFFSET = VIRT_ZONE_QUEUE_OFFSET + VIRT_ZONE_QUEUE_SIZE;
        inline constexpr size_t VIRT_ZONE_TA_SIZE = (1ull << 30); // 1GB
        inline constexpr size_t VIRT_ZONE_TA_BASELINE = (1ull << 26); // 64MB

        // COMPONENT POOL ARENA
        inline constexpr size_t VIRT_ZONE_CPA_OFFSET = VIRT_ZONE_TA_OFFSET + VIRT_ZONE_TA_SIZE;
        inline constexpr size_t VIRT_ZONE_CPA_SIZE = (1ull << 33); // 8GB
        inline constexpr size_t VIRT_ZONE_CPA_BASELINE = (1ull << 27); // 128MB

        // SCENE ARENA
        inline constexpr size_t VIRT_ZONE_SA_OFFSET = VIRT_ZONE_CPA_OFFSET + VIRT_ZONE_CPA_SIZE;
        inline constexpr size_t VIRT_ZONE_SA_SIZE = (1ull << 32); // 4GB
        inline constexpr size_t VIRT_ZONE_SA_BASELINE = (1ull << 26); // 64MB

        // IO STREAMING SPACE
        inline constexpr size_t VIRT_ZONE_IO_OFFSET = VIRT_ZONE_SA_OFFSET + VIRT_ZONE_SA_SIZE;
        inline constexpr size_t VIRT_ZONE_IO_SIZE = (1ull << 30); // 1GB (to be reconsidered)
        inline constexpr size_t VIRT_ZONE_IO_BASELINE = UINT64_MAX; // No memory commited, thus no baseline
    }

    struct EngineMemoryConfig
    {
        size_t totalPhysicalRam;
        size_t globalLockCeiling;
        size_t threadSoftBaseline;
    };

    extern EngineMemoryConfig g_MemoryConfig;

    void MemoryConfig_Initialize(size_t physicalRamInstalled, uint32_t maxEngineWorkers);
    void MemoryConfig_Shutdown();
}
