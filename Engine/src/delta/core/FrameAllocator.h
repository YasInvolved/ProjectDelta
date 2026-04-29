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

namespace delta::core
{
    namespace MemoryManager { struct MemoryState; }

    struct FrameAllocator
    {
        struct Node { Node* next; };

        MemoryManager::MemoryState* memoryState;

        Node* head;
        Node* current;

        uint64_t pageSize;
        uint64_t offset;
    };

    void FrameAllocator_Init(FrameAllocator* allocator, MemoryManager::MemoryState* memState);
    void* FrameAllocator_Allocate(FrameAllocator* allocator, uint64_t size, uint64_t alignment = 8);
    void FrameAllocator_Flush(FrameAllocator* allocator);
    void FrameAllocator_Shutdown(FrameAllocator* allocator);
}
