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

#include <iostream>

#include <delta/core/engine.h>
#include <delta/platform/os.h>
#include <delta/platform/os_internal.h>
#include <delta/core/ThreadContext.h>
#include <delta/core/MemoryConfig.h>
#include <delta/core/EngineTypes.h>

void delta::Engine::Initialize(Context& context)
{
    context.isRunning = true;
    delta::platform::Initialize();

    const auto* osInfo = delta::platform::getOSInfo();
    const auto memStatus  = delta::platform::getMemoryStatus();
    delta::core::MemoryConfig_Initialize(memStatus.physicalInstalled, osInfo->maxEngineWorkerCount);

    if (!delta::platform::Memory_ElevateLockLimit(delta::core::g_MemoryConfig.globalLockCeiling))
    {
        context.isRunning = false;
        return;
    }

    delta::core::ThreadContext_Initialize(osInfo->maxEngineWorkerCount, osInfo->osPageSize);
}

void delta::Engine::Shutdown(Context& context)
{
    delta::core::ThreadContext_Shutdown();
    delta::core::MemoryConfig_Shutdown();
}
