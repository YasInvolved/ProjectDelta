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
using BucketMetadata = FreeListAllocator::BucketMetadata;

DLT_FORCE_INLINE void* AllocatePageForBucket(FreeListAllocator* allocator, uint32_t bucketIx, uint64_t size)
{
    void* rawPage = MemoryManager::AllocatePageLockFree(allocator->memState);

    BucketMetadata* meta = reinterpret_cast<BucketMetadata*>(rawPage);
    meta->magic = BucketMetadata::MAGIC_VALUE;
    meta->bucketIx = bucketIx;

    uint64_t numChunks = allocator->remaining / size;
    uint8_t* walker = static_cast<uint8_t*>(rawPage) + sizeof(BucketMetadata);

    Node* head = reinterpret_cast<Node*>(walker);
    Node* current = head;

    for (uint32_t i = 0; i < numChunks; i++)
    {
        walker += size;
        Node* next = reinterpret_cast<Node*>(walker);
        current->next = next;
        current = next;
    }

    current->next = nullptr;
    allocator->buckets[bucketIx] = head;
    return head;
}

void delta::core::FreeList_Init(FreeListAllocator* allocator, MemoryState* memState)
{
    allocator->memState = memState;
    allocator->pageSize = memState->pageSize;
    allocator->remaining = memState->pageSize - sizeof(BucketMetadata);
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
    if (!ptr)
        return;

    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t basePage = address & ~0xfffull;

    const BucketMetadata* metadata = reinterpret_cast<const BucketMetadata*>(basePage);
    assert(metadata->magic == BucketMetadata::MAGIC_VALUE);

    uint32_t bucketIx = metadata->bucketIx;
    Node* freeNode = reinterpret_cast<Node*>(ptr);
    freeNode->next = allocator->buckets[bucketIx];
    allocator->buckets[bucketIx] = freeNode;
}
