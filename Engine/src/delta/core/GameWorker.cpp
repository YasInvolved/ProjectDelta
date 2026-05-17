#include "GameWorker.h"

#include <delta/platform/os_internal.h>

using namespace delta::core;
namespace os = delta::platform;

// static pool for workers
static uint32_t s_current;
static GameWorker s_workers[MAX_WORKERS];

static void workerLoop(void* data)
{
    while (true)
    { }
}

void delta::core::GameWorker_Init()
{
    s_current = 0u;
    memset(s_workers, 0u, sizeof(GameWorker) * MAX_WORKERS);
}

uint32_t delta::core::GameWorker_GetCurrentNumber() noexcept
{
    return s_current;
}

GameWorker* delta::core::GameWorker_Create() noexcept
{
    if (s_current == MAX_WORKERS)
        return nullptr;

    static constexpr os::ThreadCreationInfo createInfo
    {
        .entryPoint = workerLoop,
        .userData = nullptr,
        .coreAffinityMask = 0,
        .debugName = "Worker",
    };

    GameWorker& newWorker = s_workers[++s_current];
    newWorker.threadHandle = os::CreateEngineThread(createInfo);
}
