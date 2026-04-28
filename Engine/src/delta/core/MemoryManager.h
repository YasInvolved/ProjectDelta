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

namespace delta::core::MemoryManager
{
    using atomic_u64_t = std::atomic<uint64_t>;

    struct MemoryState
    {
        void* reservedBase;
        uint64_t totalVirtualSize;
        uint64_t pageSize;

        uint64_t bitmaskCount;
        atomic_u64_t* commitedBitmask;

        uint64_t maxCommitBudgetBytes;
        atomic_u64_t currentlyCommitedBytes;
    };

    void InitEngineMemory();
    void ShutdownEngineMemory();
    void* AllocatePageLockFree(MemoryState* state);
    void freePage(MemoryState* state, void* ptr);
    const MemoryState* GetMemoryState() noexcept;
}
