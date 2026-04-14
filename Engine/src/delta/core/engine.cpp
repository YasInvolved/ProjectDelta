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

void delta::Engine::Initialize(Context& context)
{
    context.isRunning = true;
    delta::platform::os::Initialize();

    const auto* sysCtx = delta::platform::os::getContext();
    std::cout <<
        "[DeltaEngine] OS Page size: " << sysCtx->config.osPageSize << '\n' <<
        "[DeltaEngine] Total RAM: " << sysCtx->memory.totalRam << " MB\n" <<
        "[DeltaEngine] Available RAM: " << sysCtx->memory.freeRam << " MB\n" <<
        "[DeltaEngine] CPU VendorID: " << sysCtx->cpu.manufacturerId << '\n' <<
        "[DeltaEngine] CPU Count: " << sysCtx->cpu.processorCount << '\n' <<
        "[DeltaEngine] AVX2 Support: " << (sysCtx->cpu.hasAVX2 ? "true" : "false") << '\n';
}

void delta::Engine::Shutdown(Context& context)
{
    // shutdown
}
