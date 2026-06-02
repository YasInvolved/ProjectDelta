#pragma once

#define DLT_DEFINE_HANDLE(name)\
    struct name;\
    using name##Handle = name*;\
    inline constexpr name##Handle INVALID_##name##_HANDLE = nullptr

namespace delta::platform
{
    DLT_DEFINE_HANDLE(Window);

    struct OSInfo
    {
        const char* cpuArchitecture;

        uint32_t cpuPhysicalCoreCount;
        uint32_t cpuLogicalProcessorCount;
        uint32_t osPageSize;

        char cpuBrandString[sizeof(int) * 12 + 1];
        char cpuManufacturerId[13];

        bool cpuHasSMT;
        bool cpuHasAVX2;
        bool cpuHasAVX512f;
        bool cpuHasAVX512cd;
        bool cpuHasAVX512er;
        bool cpuHasAVX512pf;
    };

    struct MemoryStatus
    {
        uint64_t physicalInstalled;
        uint64_t physicalFree;
    };
}
