#pragma once

#include <cstdint>
#include <cstddef>

#include <delta/definitions.h>

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

    DLT_API const OSInfo* getOSInfo() noexcept;
}
