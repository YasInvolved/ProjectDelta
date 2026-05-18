#pragma once

// forward def
namespace delta::platform
{
    struct OSThreadHandle;
}

namespace delta::core
{
    inline constexpr uint32_t MAX_WORKERS = 8u;

    struct GameWorker
    {
        delta::platform::OSThreadHandle* threadHandle;
    };

    void GameWorker_Init();
    void GameWorker_Shutdown();
}
