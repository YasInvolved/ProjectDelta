#pragma once

#include <delta/platform/os_internal.h>

namespace delta::core
{
    struct Empty_t{};

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
    };

    using task_t = void (*)(void*);
    using payload_t = void*;
    struct TaskQueue // SoA structure, Chase-Lev queue
    {
        alignas(64) std::atomic<uint64_t> top;
        alignas(64) std::atomic<uint64_t> bottom;

        alignas(64) uint64_t size;
        uint64_t mask;

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
        uint32_t threadId;

        ThreadPageCoordinator pageCoordinator;
        ThreadArena transientArena;
        delta::platform::Timer perThreadTimer;
    };

    struct alignas(64) MainExecutionContext
    {
        // SHARED TRAITS
        GenericExecutionContext generic;

        // ROLE TRAITS
        // TODO: persistent storage
    };

    struct alignas(64) WorkerExecutionContext
    {
        GenericExecutionContext generic;

        // ROLE TRAITS
        TaskQueue taskQueue;
    };

    // TODO: Remove, obsolete type.
    struct alignas(64) ThreadExecutionContext
    {
        uint32_t threadIx;
        uint32_t threadId;

        ThreadPageCoordinator pageCoordinator;
        TaskQueue taskQueue;

        alignas(64) ThreadArena transientArena;
        ThreadArena componentPoolArena;
        ThreadArena sceneArena;

        alignas(64) delta::platform::Timer perThreadTimer;

        uint8_t hardwarePadding[32];
    };

    // COMPILE TIME CONSTANTS
    inline constexpr size_t THREAD_EXECUTION_CONTEXT_STRIDE = std::max<size_t>(sizeof(MainExecutionContext), sizeof(WorkerExecutionContext));
    inline constexpr size_t THREAD_EXECUTION_CONTEXT_SIZE = (THREAD_EXECUTION_CONTEXT_STRIDE + 63) & ~(63);

    // EXTERN VARIABLES
    extern thread_local GenericExecutionContext* tl_CurrentThreadContext;

    // INLINE HELPER FUNCTIONS
    template <typename TargetType>
    [[nodiscard]] inline TargetType* ThreadContextCast(GenericExecutionContext* ctx)
    {
        if (!ctx) return nullptr;

        if constexpr (std::is_same_v<TargetType, MainExecutionContext>)
        {
            if (ctx->type == ThreadType::MAIN)
                return reinterpret_cast<TargetType*>(ctx);
        }
        else if constexpr (std::is_same_v<TargetType, WorkerExecutionContext>)
        {
            if (ctx->type == ThreadType::WORKER)
                return reinterpret_cast<TargetType*>(ctx);
        }

        assert(false); // CRITICAL: Casting to invalid type
        return nullptr;
    }

    inline bool IsMainThread()
    {
        return tl_CurrentThreadContext && tl_CurrentThreadContext->type == ThreadType::MAIN;
    }

    inline bool IsWorkerThread()
    {
        return tl_CurrentThreadContext && tl_CurrentThreadContext->type == ThreadType::WORKER;
    }
}
