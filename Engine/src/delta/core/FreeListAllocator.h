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

    struct FreeListAllocator
    {
        static constexpr uint64_t INDEX_OFFSET = 4ull; // 2^4 = 16
        static constexpr uint64_t MIN_ALLOC_SIZE = 16ull;
        static constexpr uint64_t MAX_BUCKET_SIZE = 2048ull;
        static constexpr uint32_t BUCKET_COUNT = 7ull; // log2(2048) - log2(16) = 7, calculated manually for simplicity

        struct Node { Node* next; };
        struct alignas(16) BucketMetadata
        {
            static constexpr char MAGIC_VALUE_STR[sizeof(uint64_t)] = "DLT_SFA";
            static constexpr uint64_t MAGIC_VALUE = std::bit_cast<uint64_t>(MAGIC_VALUE_STR);

            uint64_t magic;
            uint32_t bucketIx;
        };

        MemoryManager::MemoryState* memState;

        Node* buckets[BUCKET_COUNT];

        uint64_t pageSize;
        uint64_t remaining; // pageSize - metadata size
    };

    void FreeList_Init(FreeListAllocator* allocator, MemoryManager::MemoryState* memState);
    void* FreeList_Allocate(FreeListAllocator* allocator, uint64_t size, uint64_t alignment);
    void FreeList_Free(FreeListAllocator* allocator, void* ptr);
}
