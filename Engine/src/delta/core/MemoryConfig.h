/*
 * Copyright 2026 Jakub Bączyk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

namespace delta::core
{
    namespace MemoryMap
    {
        // Generic
        inline constexpr size_t VIRT_ZONE_SPACE_LENGTH = (1ull << 35);

        inline constexpr size_t VIRT_ZONE_TA_OFFSET = 0ull;
        inline constexpr size_t VIRT_ZONE_TA_SIZE = (1ull << 30); // 1GB
        inline constexpr size_t VIRT_ZONE_TA_BASELINE = (1ull << 26);

        namespace Main
        {
            inline constexpr size_t VIRT_ZONE_PS_OFFSET = VIRT_ZONE_TA_OFFSET + VIRT_ZONE_TA_SIZE;
            inline constexpr size_t VIRT_ZONE_PS_SIZE = (1ull << 31); // 2GB
            inline constexpr size_t VIRT_ZONE_PS_BASELINE = (1ull << 26); // 64MB

            inline constexpr size_t VIRT_ZONE_BASELINE_SUM = VIRT_ZONE_PS_SIZE;
        }

        namespace Worker
        {
            // QUEUE
            inline constexpr size_t VIRT_ZONE_QUEUE_OFFSET = VIRT_ZONE_TA_OFFSET + VIRT_ZONE_TA_SIZE;
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

            // BASELINE SUMMARY
            inline constexpr size_t VIRT_ZONE_BASELINE_SUM =
                VIRT_ZONE_TA_BASELINE +
                VIRT_ZONE_CPA_BASELINE +
                VIRT_ZONE_SA_BASELINE;
        }
    }

    struct EngineMemoryConfig
    {
        size_t totalPhysicalRam;
        size_t maxAllowedPhysical;
        size_t threadSoftBaseline;
        size_t activeLockAllocation;
        size_t globalPoolSize;
    };

    extern EngineMemoryConfig g_MemoryConfig;

    void MemoryConfig_Initialize(size_t physicalRamInstalled, size_t pageSize, uint32_t maxEngineWorkers);
    void MemoryConfig_Shutdown();
}
