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

#include "MemoryManager.h"

#include <delta/platform/os.h>
#include <delta/platform/os_internal.h>

#include <new>

#define SET_BIT(mask, bit) mask |= (1 << bit)

using namespace delta::core;
using MemoryState = MemoryManager::MemoryState;
namespace os = delta::platform;

// ct constants
static constexpr uint64_t VIRTUAL_MEMORY_RESERVATION = (1ull << 35ull); // 32gb

static MemoryState memoryState{};

void MemoryManager::InitEngineMemory()
{
    {
        const auto* osInfo = os::getOSInfo();
        const auto memStatus = os::getMemoryStatus();
        memoryState.pageSize = osInfo->osPageSize;
        memoryState.maxCommitBudgetBytes = (memStatus.physicalInstalled / 100) * 7; // ~70% of total capacity
        memoryState.totalVirtualSize = VIRTUAL_MEMORY_RESERVATION;
    }

    uint64_t totalPages = VIRTUAL_MEMORY_RESERVATION / memoryState.pageSize;
    memoryState.bitmaskCount = (totalPages + 63) / 64;

    uint64_t bitmaskBytes = memoryState.bitmaskCount * sizeof(atomic_u64_t);

    void* rawBitmaskPtr = os::ReserveMemory(bitmaskBytes);
    rawBitmaskPtr = os::CommitMemory(rawBitmaskPtr, bitmaskBytes);
    assert(rawBitmaskPtr != nullptr); // failed to allocate metadata

    memoryState.commitedBitmask = static_cast<atomic_u64_t*>(rawBitmaskPtr);
    for (uint64_t i = 0; i < memoryState.bitmaskCount; i++)
    {
        new (&memoryState.commitedBitmask[i]) atomic_u64_t(~0ull);
    }

    memoryState.reservedBase = os::ReserveMemory(memoryState.totalVirtualSize);
    assert(memoryState.reservedBase != nullptr); // failed to reserve addresses for allocations
}

static void* allocatePageLockFree(MemoryState* state)
{
    uint64_t currentRAM = state->currentlyCommitedBytes.load(std::memory_order_relaxed);
    while (true)
    {
        if (currentRAM + state->pageSize > state->maxCommitBudgetBytes)
            return nullptr;

        if (state->currentlyCommitedBytes.compare_exchange_weak(
            currentRAM, currentRAM + state->pageSize,
            std::memory_order_acquire, std::memory_order_relaxed)
        )
            break;
    }

    for (uint64_t i = 0; i < state->bitmaskCount; i++)
    {
        uint64_t expected = state->commitedBitmask[i].load(std::memory_order_relaxed);
        while (expected != 0)
        {
            int bitIx = std::countr_zero(expected);
            uint64_t desired = expected & ~(1ull << bitIx);

            if (state->commitedBitmask[i].compare_exchange_weak(
                expected, desired,
                std::memory_order_acquire, std::memory_order_relaxed))
            {
                size_t absolutePageIndex = (i * 64) + bitIx;
                uintptr_t targetPtr = reinterpret_cast<uintptr_t>(state->reservedBase) + (absolutePageIndex * state->pageSize);
                void* commitedMemory = os::CommitMemory(reinterpret_cast<void*>(targetPtr), state->pageSize);
                assert(commitedMemory != nullptr); // os failed to commit page
                return commitedMemory;
            }
        }
    }
}

static void freePage(MemoryState* state, void* ptr)
{
    if (!ptr)
        return;

    uintptr_t offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(state->reservedBase);
    size_t absolutePageIx = offset / state->pageSize;

    size_t arrIx = absolutePageIx / 64;
    size_t bitIx = absolutePageIx % 64;

    os::DecommitMemory(ptr, state->pageSize);
    state->commitedBitmask[arrIx].fetch_or((1ull << bitIx), std::memory_order_release);
    state->currentlyCommitedBytes.fetch_sub(state->pageSize, std::memory_order_release);
}

void MemoryManager::ShutdownEngineMemory()
{
    os::ReleaseMemory(memoryState.reservedBase);
    os::ReleaseMemory(memoryState.commitedBitmask);
}
