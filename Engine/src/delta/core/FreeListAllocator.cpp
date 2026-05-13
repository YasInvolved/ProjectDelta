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

#include "FreeListAllocator.h"
#include "MemoryManager.h"

#define ALIGN(size, alignment) ((size + (alignment - 1)) & ~(size - 1))

using namespace delta::core;

using MemoryState = MemoryManager::MemoryState;
using Node = FreeListAllocator::Node;

DLT_FORCE_INLINE void* AllocatePageForBucket(FreeListAllocator* allocator, uint32_t bucketIx, uint64_t size)
{
    void* rawPage = MemoryManager::AllocatePageLockFree(allocator->memState);

    uint64_t numChunks = allocator->pageSize / size;
    uint8_t* walker = static_cast<uint8_t*>(rawPage);

    // first step
    Node* head = reinterpret_cast<Node*>(walker);
    Node* current = head;

    // next n - 1 steps
    for (uint32_t i = 0; i < numChunks - 2; i++)
    {
        walker += size;
        Node* next = reinterpret_cast<Node*>(walker);
        current->next = next;
        current = next;
    }

    walker += size;
    Node* next = reinterpret_cast<Node*>(walker);
    return next;
}

void delta::core::FreeList_Init(FreeListAllocator* allocator, MemoryState* memState)
{
    allocator->memState = memState;
    allocator->pageSize = memState->pageSize;
}

void* delta::core::FreeList_Allocate(FreeListAllocator* allocator, uint64_t size, uint64_t alignment)
{
    uint64_t aligned = ALIGN(size, alignment);

    if (aligned < FreeListAllocator::MIN_ALLOC_SIZE)
        aligned = FreeListAllocator::MIN_ALLOC_SIZE;

    if (aligned <= FreeListAllocator::MAX_BUCKET_SIZE)
    {
        uint32_t log = std::bit_width(aligned - 1);
        uint32_t bucketIx = log - FreeListAllocator::INDEX_OFFSET;

        Node* bucket = allocator->buckets[bucketIx];
        if (bucket)
        {
            allocator->buckets[bucketIx] = bucket->next;
            return bucket;
        }

        return AllocatePageForBucket(allocator, bucketIx, aligned);
    }

    return MemoryManager::AllocatePageLockFree(allocator->memState);
}

void delta::core::FreeList_Free(FreeListAllocator* allocator, void* ptr)
{

}

void delta::core::FreeList_Destroy(FreeListAllocator* allocator)
{

}
