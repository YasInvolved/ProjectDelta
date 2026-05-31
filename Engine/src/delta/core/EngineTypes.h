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

#include <delta/platform/os_internal.h>

namespace delta::core
{
    // TODO: revise this structure and its purpose
    struct ThreadPageCoordinator
    {
        uint8_t* virtualAddressBase;
        size_t commitedOffset;
        size_t reservedCapacity;
        size_t pageSize;
    };

    struct ThreadArena
    {
        uint8_t* backingMemory;
        size_t capacity;
        size_t offset;
        size_t maxCapacity;
    };

    using task_t = void (*)(void*);
    using payload_t = void*;
    using queue_index_t = int64_t;
    struct TaskQueue // SoA structure, Chase-Lev queue
    {
        alignas(64) std::atomic<queue_index_t> top;
        alignas(64) std::atomic<queue_index_t> bottom;

        alignas(64) queue_index_t size;
        queue_index_t mask;

        task_t* tasks;
        payload_t* payloads;

        static inline constexpr size_t FIELD_SIZE = sizeof(task_t) + sizeof(payload_t);
    };

    enum class ThreadType : uint8_t { MAIN, WORKER };

    struct alignas(64) GenericExecutionContext
    {
        // SHARED TRAITS
        ThreadType type;
        uint32_t threadIx;
        delta::platform::ThreadHandle threadHandle;

        ThreadPageCoordinator pageCoordinator;
        ThreadArena transientArena;
        delta::platform::Timer perThreadTimer;
    };

    struct alignas(64) MainExecutionContext
    {
        // SHARED TRAITS
        GenericExecutionContext generic;

        // ROLE TRAITS
        ThreadArena persistentStorage;
    };

    struct alignas(64) WorkerExecutionContext
    {
        GenericExecutionContext generic;

        // ROLE TRAITS
        TaskQueue taskQueue;
        delta::platform::SemaphoreHandle sleepSemaphore;
        std::atomic<bool> isAsleep;
        std::atomic<bool> shouldClose;
    };

    template <typename T>
    concept ExecutionContext =
        std::is_same_v<std::remove_cvref_t<T>, GenericExecutionContext> ||
        std::is_same_v<std::remove_cvref_t<T>, MainExecutionContext> ||
        std::is_same_v<std::remove_cvref_t<T>, WorkerExecutionContext>;

    // COMPILE TIME CONSTANTS
    inline constexpr size_t THREAD_EXECUTION_CONTEXT_STRIDE = std::max<size_t>(sizeof(MainExecutionContext), sizeof(WorkerExecutionContext));
    inline constexpr size_t THREAD_EXECUTION_CONTEXT_SIZE = (THREAD_EXECUTION_CONTEXT_STRIDE + 63) & ~(63);

    // EXTERN VARIABLES & FUNCTIONS
    extern uintptr_t g_MasterSlabStart;
    extern uintptr_t g_MasterSlabEnd;
}
