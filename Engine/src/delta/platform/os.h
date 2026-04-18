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

#include <cstddef>
#include <cstdint>

namespace delta::platform::os
{
    struct Context
    {
        struct
        {
            char manufacturerId[13];
            char brandString[sizeof(int) * 4 * 3 + 1];
            uint32_t processorCount;
            bool hasAVX2;
        } cpu;

        struct
        {
            uint64_t totalRam;
            uint64_t freeRam;
        } memory;

        struct
        {
            uint32_t osPageSize;
        } config;
    };

    void Initialize();
    const Context* getContext() noexcept;
}
