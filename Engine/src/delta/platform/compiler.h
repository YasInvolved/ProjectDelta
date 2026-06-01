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

#if defined(_MSC_VER)
    #define DLT_DISABLE_WARNING_PUSH    __pragma(warning(push))
    #define DLT_DISABLE_WARNING_POP     __pragma(warning(pop))
    #define DLT_DISABLE_MISSING_RETURN  __pragma(warning(disable : 4715))

    #define DLT_FORCE_INLINE            __forceinline

    #define DLT_UNREACHABLE             __assume(0)
#elif defined(__clang__)
    #define DLT_DISABLE_WARNING_PUSH    _Pragma("clang diagnostic push")
    #define DLT_DISABLE_WARNING_POP     _Pragma("clang diagnostic pop")
    #define DLT_DISABLE_MISSING_RETURN  _Pragma("clang diagnostic ignored \"-Wreturn-type\"")

    #define DLT_FORCE_INLINE            __attribute__((always_inline))

    #define DLT_UNREACHABLE             __builtin_unreachable()
#elif defined(__GNUC__) || defined(__GNUG__)
    #define DLT_DISABLE_WARNING_PUSH    _Pragma("GCC diagnostic push")
    #define DLT_DISABLE_WARNING_POP     _Pragma("GCC diagnostic pop")
    #define DLT_DISABLE_MISSING_RETURN  _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")

    #define DLT_FORCE_INLINE            __attribute__((always_inline))

    #define DLT_UNREACHABLE             __builtin_unreachable()
#else
    #define DLT_DISABLE_WARNING_PUSH  
    #define DLT_DISABLE_WARNING_POP   
    #define DLT_DISABLE_MISSING_RETURN

    #define DLT_FORCE_INLINE

    #define DLT_UNREACHABLE do {} while(0)
#endif
