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

        g_osInfo.osPageSize = info.dwPageSize;
        g_osInfo.cpuCoreCount = info.dwNumberOfProcessors;

        //MEMORYSTATUSEX statex{};
        //statex.dwLength = sizeof(statex);

        //if (GlobalMemoryStatusEx(&statex))
        //{
        //    g_context.memory.totalRam = statex.ullTotalPhys / (1 << 20);
        //    g_context.memory.freeRam = statex.ullAvailPhys / (1 << 20);
        //}
    }

    const OSInfo* getOSInfo() noexcept { return &g_osInfo; }
}

#endif
