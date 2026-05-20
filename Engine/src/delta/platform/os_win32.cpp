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

#ifdef _WIN32

#include <delta/platform/os.h>
#include "os_internal.h"
#include <Windows.h>
#include <intrin.h>

#define CHECK_CPUID_FLAG(register, flag) ((register & (1 << flag)) != 0)

namespace delta::platform
{
    static OSInfo g_osInfo;

    struct BrandStringCall
    {
        int c1[4];
        int c2[4];
        int c3[4];

        static constexpr char UNSPECIFIED_VALUE[] = "(unspecified)";
    };

    struct Timer_Internal
    {
        int64_t freq;
        int64_t baseStartTime;
    };

    enum CpuArchitecture : WORD
    {
        INTEL = 0,
        ARM = 5,
        IA64 = 6,
        AMD64 = 9,
        ARM64 = 12,
        UNKNOWN = 0xffff
    };

    static inline const char* getArchitectureString(WORD arch)
    {
        switch (arch)
        {
        case INTEL:
            return "x86";
        case ARM:
            return "ARM";
        case IA64:
            return "IA64";
        case AMD64:
            return "AMD64";
        case ARM64:
            return "ARM64";
        default:
            return "UNKNOWN";
        }
    }

    inline static void fetchCpuidValues()
    {
        int cpuinfo[4];
        __cpuid(cpuinfo, 0);

        memcpy(g_osInfo.cpuManufacturerId, &cpuinfo[1], sizeof(int));
        memcpy(g_osInfo.cpuManufacturerId + sizeof(int), &cpuinfo[3], sizeof(int));
        memcpy(g_osInfo.cpuManufacturerId + 2 * sizeof(int), &cpuinfo[2], sizeof(int));

        __cpuidex(cpuinfo, 7, 0);
        g_osInfo.cpuHasAVX2 = CHECK_CPUID_FLAG(cpuinfo[1], 5);
        g_osInfo.cpuHasAVX512f = CHECK_CPUID_FLAG(cpuinfo[1], 16);
        g_osInfo.cpuHasAVX512cd = CHECK_CPUID_FLAG(cpuinfo[1], 28);
        g_osInfo.cpuHasAVX512er = CHECK_CPUID_FLAG(cpuinfo[1], 27);
        g_osInfo.cpuHasAVX512pf = CHECK_CPUID_FLAG(cpuinfo[1], 26);
        

        __cpuid(cpuinfo, 0x80000000);
        if (cpuinfo[0] >= 0x80000004)
        {
            BrandStringCall* c = reinterpret_cast<BrandStringCall*>(g_osInfo.cpuBrandString);
            __cpuid(c->c1, 0x80000002);
            __cpuid(c->c2, 0x80000003);
            __cpuid(c->c3, 0x80000004);
        }
        else
            memcpy(g_osInfo.cpuBrandString, BrandStringCall::UNSPECIFIED_VALUE, sizeof(BrandStringCall::UNSPECIFIED_VALUE));
    }

    void Initialize()
    {
        memset(&g_osInfo, 0u, sizeof(OSInfo));
        fetchCpuidValues();

        SYSTEM_INFO info;
        GetSystemInfo(&info);

        g_osInfo.cpuArchitecture = getArchitectureString(static_cast<CpuArchitecture>(info.wProcessorArchitecture)); // safe, points to static string literal
        g_osInfo.osPageSize = info.dwPageSize;
        g_osInfo.cpuLogicalProcessorCount = info.dwNumberOfProcessors;

        DWORD bufferSize = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize);

        void* tempBuffer = malloc(bufferSize);
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX coreInfo = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(tempBuffer);

        uint32_t physicalCores = 0;
        uint32_t logicalProcessors = 0;

        if (tempBuffer && GetLogicalProcessorInformationEx(RelationProcessorCore, coreInfo, &bufferSize))
        {
            uint8_t* ptr = reinterpret_cast<uint8_t*>(tempBuffer);
            while (ptr < reinterpret_cast<uint8_t*>(tempBuffer) + bufferSize)
            {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

                if (current->Relationship == RelationProcessorCore)
                {
                    physicalCores++;

                    for (DWORD g = 0; g < current->Processor.GroupCount; g++)
                    {
                        logicalProcessors += __popcnt64(current->Processor.GroupMask[g].Mask);
                    }
                }

                ptr += current->Size;
            }
        }

        free(tempBuffer);

        g_osInfo.cpuPhysicalCoreCount = physicalCores;
        g_osInfo.cpuLogicalProcessorCount = logicalProcessors;
        g_osInfo.cpuHasSMT = (logicalProcessors > physicalCores);

        if (physicalCores > 1)
            g_osInfo.maxEngineWorkerCount = physicalCores - 1;
        else
            g_osInfo.maxEngineWorkerCount = 1;
    }

    void* ReserveMemory(size_t reservationSize)
    {
        // we don't allow accessing uncommited memory
        void* ptr = VirtualAlloc(nullptr, reservationSize, MEM_RESERVE, PAGE_NOACCESS);
        return ptr;
    }

    void* CommitMemory(void* mem, size_t commitSize)
    {
        void* ptr = VirtualAlloc(mem, commitSize, MEM_COMMIT, PAGE_READWRITE);
        return ptr;
    }

    void DecommitMemory(void* mem, size_t decommitSize)
    {
        VirtualFree(mem, decommitSize, MEM_DECOMMIT);
    }

    void ReleaseMemory(void* ptr)
    {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }

    const OSInfo* getOSInfo() noexcept { return &g_osInfo; }

    MemoryStatus getMemoryStatus()
    {
        MemoryStatus status{};

        MEMORYSTATUSEX wstatus{ .dwLength = sizeof(MEMORYSTATUSEX) };
        if (GlobalMemoryStatusEx(&wstatus))
        {
            status.physicalInstalled = wstatus.ullTotalPhys;
            status.physicalFree = wstatus.ullAvailPhys;
        }

        return status;
    }

    // Timer
    static_assert(sizeof(Timer_Internal) <= sizeof(Timer), "Timer opaque storage is too small!");

    void Timer_Initialize(Timer* timer)
    {
        Timer_Internal* internal = reinterpret_cast<Timer_Internal*>(timer->opaqueData);

        LARGE_INTEGER freq;
        BOOL freqResult = QueryPerformanceFrequency(&freq);
        assert(freqResult && "Hardware high-performance counter is not supported by this system");
        internal->freq = freq.QuadPart;

        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
        internal->baseStartTime = start.QuadPart;
    }

    int64_t Timer_GetTimestamp()
    {
        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        return current.QuadPart;
    }

    double Timer_TicksToMilliseconds(const Timer* timer, int64_t startTicks, int64_t endTicks)
    {
        const Timer_Internal* internal = reinterpret_cast<const Timer_Internal*>(timer);
        int64_t elapsedTicks = endTicks - startTicks;

        return (static_cast<double>(elapsedTicks) * 1000.0) / static_cast<double>(internal->freq);
    }

    double Timer_TicksToMicroseconds(const Timer* timer, int64_t startTicks, int64_t endTicks)
    {
        const Timer_Internal* internal = reinterpret_cast<const Timer_Internal*>(timer);
        int64_t elapsedTicks = endTicks - startTicks;

        return (static_cast<double>(elapsedTicks) * 1000000.0) / static_cast<double>(internal->freq);
    }
}

#endif
