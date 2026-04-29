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

#include "FrameAllocator.h"
#include <delta/core/MemoryManager.h>

#define ALIGN_OFFSET(offset, alignment) ((offset + (alignment - 1)) & ~(alignment - 1))

using FrameAllocator = delta::core::FrameAllocator;
using Node = FrameAllocator::Node;

static DLT_FORCE_INLINE void* allocate(FrameAllocator* allocator, uint64_t size, uint64_t alignedOffset)
{
    void* result = reinterpret_cast<uint8_t*>(allocator->current) + alignedOffset;
    allocator->offset = size + alignedOffset;
    return result;
}

void delta::core::FrameAllocator_Init(FrameAllocator* allocator, MemoryManager::MemoryState* memState)
{
    allocator->memoryState = memState;
    allocator->pageSize = memState->pageSize;

    void* rawPage = MemoryManager::AllocatePageLockFree(memState);
    assert(rawPage != nullptr); // failed to allocate a page

    allocator->head = static_cast<Node*>(rawPage);
    allocator->head->next = nullptr;

    allocator->current = allocator->head;
    allocator->offset = sizeof(Node);
}

void* delta::core::FrameAllocator_Allocate(FrameAllocator* allocator, uint64_t size, uint64_t alignment)
{
    uint64_t alignedOffset = ALIGN_OFFSET(allocator->offset, alignment);

    if (alignedOffset + size <= allocator->pageSize)
        return allocate(allocator, size, alignedOffset);

    if (allocator->current->next != nullptr)
    {
        allocator->current = allocator->current->next;
    }
    else
    {
        void* newPage = MemoryManager::AllocatePageLockFree(allocator->memoryState);
        assert(newPage != nullptr); // failed to allocate a page

        Node* node = static_cast<Node*>(newPage);
        node->next = nullptr;

        allocator->current->next = node;
        allocator->current = node;
    }

    allocator->offset = sizeof(Node);
    alignedOffset = ALIGN_OFFSET(allocator->offset, alignment);

    return allocate(allocator, size, alignedOffset);
}

void delta::core::FrameAllocator_Flush(FrameAllocator* allocator)
{
    allocator->current = allocator->head;
    allocator->offset = sizeof(Node);
}

void delta::core::FrameAllocator_Shutdown(FrameAllocator* allocator)
{
    Node* current = allocator->head;
    while (current != nullptr)
    {
        Node* n = current->next;
        MemoryManager::freePage(allocator->memoryState, current);
        current = n;
    }
}
