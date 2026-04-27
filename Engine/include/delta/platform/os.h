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

namespace delta::platform
{
    struct OSInfo
    {
        // cpu
        char cpuManufacturerId[13];
        char cpuBrandString[sizeof(int) * 12 + 1];
        const char* cpuArchitecture;
        uint32_t cpuCoreCount;
        bool cpuHasAVX2;
        bool cpuHasAVX512f;
        bool cpuHasAVX512cd;
        bool cpuHasAVX512er;
        bool cpuHasAVX512pf;

        // general OS info
        uint32_t osPageSize;
    };

    struct MemoryStatus
    {
        uint64_t physicalInstalled;
        uint64_t physicalFree;
    };

    DLT_API const OSInfo* getOSInfo() noexcept;
    DLT_API MemoryStatus getMemoryStatus();
}
