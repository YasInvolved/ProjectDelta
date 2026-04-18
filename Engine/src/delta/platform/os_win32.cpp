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

#include "os.h"
#include <Windows.h>
#include <intrin.h>

namespace delta::platform::os
{
    static Context g_context;

    struct BrandStringCall
    {
        int c1[4];
        int c2[4];
        int c3[4];

        static constexpr char UNSPECIFIED_VALUE[] = "(unspecified)";
    };

    inline static void fetchCpuidValues()
    {
        memset(g_context.cpu.manufacturerId, 0, 13);

        int cpuinfo[4];
        __cpuid(cpuinfo, 0);

        memcpy(g_context.cpu.manufacturerId, &cpuinfo[1], sizeof(int));
        memcpy(g_context.cpu.manufacturerId + 4, &cpuinfo[3], sizeof(int));
        memcpy(g_context.cpu.manufacturerId + 8, &cpuinfo[2], sizeof(int));

        __cpuidex(cpuinfo, 7, 0);
        g_context.cpu.hasAVX2 = (cpuinfo[1] & (1 << 5)) != 0;

        __cpuid(cpuinfo, 0x80000000);
        if (cpuinfo[0] >= 0x80000004)
        {
            BrandStringCall* c = reinterpret_cast<BrandStringCall*>(g_context.cpu.brandString);
            __cpuid(c->c1, 0x80000002);
            __cpuid(c->c2, 0x80000003);
            __cpuid(c->c3, 0x80000004);
        }
        else
            memcpy(g_context.cpu.brandString, BrandStringCall::UNSPECIFIED_VALUE, sizeof(BrandStringCall::UNSPECIFIED_VALUE));
    }

    void Initialize()
    {
        fetchCpuidValues();

        SYSTEM_INFO info;
        GetSystemInfo(&info);

        g_context.config.osPageSize = info.dwPageSize;
        g_context.cpu.processorCount = info.dwNumberOfProcessors;

        MEMORYSTATUSEX statex{};
        statex.dwLength = sizeof(statex);

        if (GlobalMemoryStatusEx(&statex))
        {
            g_context.memory.totalRam = statex.ullTotalPhys / (1 << 20);
            g_context.memory.freeRam = statex.ullAvailPhys / (1 << 20);
        }
    }

    const Context* getContext() noexcept { return &g_context; }

    static_assert(sizeof(BrandStringCall) <= sizeof(g_context.cpu.brandString));
    static_assert(sizeof(BrandStringCall::UNSPECIFIED_VALUE) <= sizeof(g_context.cpu.brandString));
}

#endif
