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

#include <delta/platform/compiler.h>
#include <delta/platform/os.h>
#include "os_internal.h"
#include <Windows.h>
#include <intrin.h>
#include <iostream>

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

    struct Thread
    {
        HANDLE hThread;
    };

    struct Semaphore
    {
        HANDLE hSemaphore;
    };

    struct THREADNAME_INFO
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };

    static_assert(std::is_standard_layout_v<Thread>, "PAL layout broken: Must be standard layout!");
    static_assert(sizeof(Thread) == sizeof(HANDLE), "PAL layout broken: Size mismatch!");

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

    DLT_FORCE_INLINE static void fetchCpuidValues()
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

    DLT_FORCE_INLINE static bool SetProcessPrivileges()
    {
        HANDLE hToken = NULL;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return false;

        TOKEN_PRIVILEGES tp;
        LUID luid;
        if (!LookupPrivilegeValueA(NULL, SE_INC_WORKING_SET_NAME, &luid))
        {
            CloseHandle(hToken);
            return false;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        DWORD error = GetLastError();

        CloseHandle(hToken);
        if (result == FALSE || error == ERROR_NOT_ALL_ASSIGNED)
            return false;

        return true;
    }

    // TODO: Move this variables to the reasonable place
    static HINSTANCE s_hInstance = nullptr;
    static STARTUPINFOA s_StartupInfo = { sizeof(STARTUPINFOA) };

    void Initialize()
    {
        memset(&g_osInfo, 0u, sizeof(OSInfo));
        fetchCpuidValues();

        s_hInstance = GetModuleHandleA(nullptr);
        GetStartupInfoA(&s_StartupInfo);

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

        bool privilegesSet = SetProcessPrivileges();
        if (!privilegesSet)
            std::cout << "[DeltaEngine-Warning] Failed to elevate SE_INC_WORKING_SET_NAME privilege. You may want to run the game as an administrator.\n";
    }

    void* Memory_Reserve(size_t reservationSize)
    {
        // we don't allow accessing uncommited memory
        void* ptr = VirtualAlloc(nullptr, reservationSize, MEM_RESERVE, PAGE_NOACCESS);
        return ptr;
    }

    void* Memory_Commit(void* mem, size_t commitSize)
    {
        void* ptr = VirtualAlloc(mem, commitSize, MEM_COMMIT, PAGE_READWRITE);
        return ptr;
    }

    void Memory_Decommit(void* mem, size_t decommitSize)
    {
        VirtualFree(mem, decommitSize, MEM_DECOMMIT);
    }

    void Memory_Release(void* ptr)
    {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }

    bool Memory_Lock(void* mem, size_t bytes)
    {
        return VirtualLock(mem, bytes);
    }

    bool Memory_ElevateLockLimit(size_t bytesToLock)
    {
        HANDLE hProcess = GetCurrentProcess();
        size_t minWorkingSet = 0;
        size_t maxWorkingSet = 0;

        if (GetProcessWorkingSetSize(hProcess, &minWorkingSet, &maxWorkingSet))
        {
            static constexpr size_t SAFETY_BUFFER_SIZE = (1ull << 26);
            size_t newMin = minWorkingSet + bytesToLock;
            size_t newMax = maxWorkingSet + bytesToLock;
            return SetProcessWorkingSetSizeEx(hProcess, newMin, newMax, QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_ENABLE) == TRUE;
        }

        return false;
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

    static DWORD WINAPI DeltaThreadProc(LPVOID lParam)
    {
        ThreadCreateInfo* createInfo = reinterpret_cast<ThreadCreateInfo*>(lParam);
        createInfo->fn(createInfo->args);
        return 0;
    }

    uint32_t Thread_GetCurrentId()
    {
        return GetCurrentThreadId();
    }

    uint32_t Thread_GetId(ThreadHandle thread)
    {
        return GetThreadId(thread->hThread);
    }

    ThreadHandle Thread_GetCurrentHandle()
    {
        Thread* t = new(delta::Engine::AllocationType::PERSISTENT) Thread();
        t->hThread = GetCurrentThread();
        if (!t->hThread)
            return nullptr;

        return t;
    }

    void Thread_AssignPhysicalCore(ThreadHandle thread, uint32_t coreIndex)
    {
        uint32_t logicalCore = coreIndex * 2;
        DWORD_PTR affinityMask = (DWORD_PTR)(1ull << logicalCore);
        DWORD_PTR prevMask = SetThreadAffinityMask(thread->hThread, affinityMask);
    }

    void Thread_SetName(ThreadHandle handle, const char* name)
    {
        const THREADNAME_INFO info =
        {
            .dwType = 0x1000,
            .szName = name,
            .dwThreadID = GetThreadId(handle->hThread),
            .dwFlags = 0
        };

        __try
        {
            ::RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<const ULONG_PTR*>(&info));
        }
        __except(EXCEPTION_CONTINUE_EXECUTION)
        {

        }
    }

    ThreadHandle Thread_Create(ThreadCreateInfo* createInfo)
    {
        Thread* thread = new(delta::Engine::AllocationType::PERSISTENT) Thread{};
        thread->hThread = CreateThread(nullptr, 0, DeltaThreadProc, (void*)createInfo, 0, 0);
        if (thread->hThread == 0)
            return nullptr;

        return static_cast<ThreadHandle>(thread);
    }

    void Thread_Join(ThreadHandle thread)
    {
        if (!thread)
            return;

        DWORD waitResult = WaitForSingleObject(thread->hThread, INFINITE);
        CloseHandle(thread->hThread);
    }

    void Thread_JoinMultiple(ThreadHandle* threads, uint32_t count)
    {
        if (!threads)
            return;

        WaitForMultipleObjects(count, reinterpret_cast<HANDLE*>(threads), TRUE, INFINITE);
    }

    SemaphoreHandle Sync_CreateSemaphore()
    {
        Semaphore* s = new(delta::Engine::AllocationType::PERSISTENT) Semaphore{};
        s->hSemaphore = CreateSemaphoreA(nullptr, 0, 1000000, nullptr);
        if (s->hSemaphore == nullptr)
            return nullptr;
        
        return static_cast<SemaphoreHandle>(s);
    }

    void Sync_DestroySemaphore(SemaphoreHandle sem)
    {
        if (!sem)
            return;

        CloseHandle(sem->hSemaphore);
    }

    void Sync_SignalSemaphore(SemaphoreHandle handle)
    {
        ReleaseSemaphore(handle->hSemaphore, 1, nullptr);
    }

    void Sync_WaitSemaphore(SemaphoreHandle handle)
    {
        WaitForSingleObject(handle->hSemaphore, INFINITE);
    }

    void Sync_Sleep(uint32_t milliseconds)
    {
        Sleep(milliseconds);
    }

    // ------------------------------------------ WINDOW API ------------------------------------------

    struct Window
    {
        HWND hwnd;
    };

    static constexpr const char MAIN_WND_CLASS_NAME[] = "DltWindow";

    static LRESULT CALLBACK DltWindowProc(HWND hwnd, UINT umesg, WPARAM wparam, LPARAM lparam)
    {
        switch (umesg)
        {
        case WM_CLOSE:
            // DestroyWindow(hwnd);
            return 0;
        default:
            return DefWindowProcA(hwnd, umesg, wparam, lparam);
        }
    }

    Window* Window_Create()
    {
        assert(s_hInstance);
        Window* window = new(delta::Engine::AllocationType::PERSISTENT) Window();

        int nCmdShow = (s_StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ? s_StartupInfo.wShowWindow : SW_SHOWDEFAULT;

        STARTUPINFOA startupInfo = { sizeof(STARTUPINFOA) };
        GetStartupInfoA(&startupInfo);
        int nCmdShow = (startupInfo.dwFlags & STARTF_USESHOWWINDOW) ? startupInfo.wShowWindow : SW_SHOWDEFAULT;

        const WNDCLASSEXA wc =
        {
            .cbSize = sizeof(WNDCLASSEXA),
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = DltWindowProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = s_hInstance,
            .hCursor = LoadCursorA(nullptr, IDC_ARROW),
            .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
            .lpszClassName = MAIN_WND_CLASS_NAME
        };

        if (!RegisterClassExA(&wc))
        {
            [[maybe_unused]] DWORD e = GetLastError();
            return nullptr;
        }

        uint32_t clientWidth = 1280;
        uint32_t clientHeight = 720;

        RECT wr = { 0, 0, static_cast<LONG>(clientWidth), static_cast<LONG>(clientHeight) };
        DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

        AdjustWindowRect(&wr, windowStyle, FALSE);

        window->hwnd = CreateWindowExA(
            0,
            MAIN_WND_CLASS_NAME,
            "Delta Engine",
            windowStyle,
            CW_USEDEFAULT, CW_USEDEFAULT,
            wr.right - wr.left,
            wr.bottom - wr.top,
            nullptr,
            nullptr,
            s_hInstance,
            nullptr
        );

        if (!window->hwnd)
        {
            return nullptr;
        }

        ShowWindow(window->hwnd, nCmdShow);
        UpdateWindow(window->hwnd);

        return window;
    }
}

#endif
