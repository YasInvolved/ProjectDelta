#include "GameWorker.h"

#include <delta/platform/os_internal.h>

using namespace delta::core;
namespace os = delta::platform;

// static pool for workers
static GameWorker s_workers[MAX_WORKERS];
static std::atomic<bool> s_shutdown;

static void workerLoop(void* data)
{
    bool shutdown = false;
    while (!shutdown)
    {
        shutdown = s_shutdown.load(std::memory_order_acquire);
    }
}

static DLT_FORCE_INLINE void createWorker(uint32_t ix) noexcept
{
    static constexpr os::ThreadCreationInfo createInfo
    {
        .entryPoint = workerLoop,
        .userData = nullptr,
        .coreAffinityMask = 0,
        .debugName = "Worker",
    };

    GameWorker& newWorker = s_workers[ix];
    newWorker.threadHandle = os::CreateEngineThread(createInfo);
}

void delta::core::GameWorker_Init()
{
    memset(s_workers, 0u, sizeof(GameWorker) * MAX_WORKERS);
    s_shutdown.store(false, std::memory_order_relaxed);

    for (uint32_t i = 0; i < MAX_WORKERS; i++)
        createWorker(i);
}

void delta::core::GameWorker_Shutdown()
{
    s_shutdown.store(true, std::memory_order_release);

    for (uint32_t i = 0; i < MAX_WORKERS; i++)
        os::DestroyEngineThread(s_workers[i].threadHandle);
}
