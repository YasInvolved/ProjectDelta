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

#ifdef __linux__
#error "Linux API is currently under maintanance and is not available for a short while"

#include <delta/platform/os.h>

#include <cstring>
#include <unistd.h>
#include <cpuid.h>
#include <sys/sysinfo.h>

namespace delta::platform::os
{
    static Context g_context;

    inline static void fetchCpuidValues()
    {
        memset(g_context.cpu.manufacturerId, 0, 13);

        uint32_t eax, ebx, ecx, edx;
        if (__get_cpuid(0, &eax, &ebx, &ecx, &edx))
        {
            memcpy(g_context.cpu.manufacturerId, &ebx, sizeof(uint32_t));
            memcpy(g_context.cpu.manufacturerId + 4, &edx, sizeof(uint32_t));
            memcpy(g_context.cpu.manufacturerId + 8, &ecx, sizeof(uint32_t));
        }

        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx))
            g_context.cpu.hasAVX2 = (ebx & (1 << 5)) != 0;
    }

    void Initialize()
    {
        fetchCpuidValues();

        struct sysinfo info;
        if (sysinfo(&info) == 0)
        {
            g_context.memory.totalRam = (info.totalram * info.mem_unit) / (1 << 20);
            g_context.memory.freeRam = (info.freeram * info.mem_unit) / (1 << 20);
        }

        g_context.cpu.processorCount = sysconf(_SC_NPROCESSORS_ONLN);
        g_context.config.osPageSize = getpagesize();
    }

    const Context* getContext() noexcept { return &g_context; }
}

#endif
