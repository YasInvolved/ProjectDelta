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

#include <delta/definitions.h>

namespace delta::Engine
{
    enum class AllocationType : uint8_t { TRANSIENT, PERSISTENT };

    [[nodiscard]] DLT_API void* Allocate(size_t size, AllocationType type, size_t alignment = 8) noexcept;
    DLT_API void Free(void* ptr) noexcept;
}

[[nodiscard]] inline void* operator new(size_t size) { return delta::Engine::Allocate(size, delta::Engine::AllocationType::PERSISTENT); }
[[nodiscard]] inline void* operator new(size_t size, delta::Engine::AllocationType type) { return delta::Engine::Allocate(size, type); }
[[nodiscard]] inline void* operator new[](size_t size) { return delta::Engine::Allocate(size, delta::Engine::AllocationType::PERSISTENT); }
[[nodiscard]] inline void* operator new[](size_t size, delta::Engine::AllocationType type) { return delta::Engine::Allocate(size, type); }

inline void operator delete(void* ptr) { return delta::Engine::Free(ptr); }
inline void operator delete(void* ptr, delta::Engine::AllocationType) { return delta::Engine::Free(ptr); }
inline void operator delete[](void* ptr) { return delta::Engine::Free(ptr); }
inline void operator delete[](void* ptr, delta::Engine::AllocationType) { return delta::Engine::Free(ptr); }
