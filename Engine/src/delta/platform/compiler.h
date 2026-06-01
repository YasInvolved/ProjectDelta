#pragma once

#if defined(_MSC_VER)
    #define DLT_DISABLE_WARNING_PUSH    __pragma(warning(push))
    #define DLT_DISABLE_WARNING_POP     __pragma(warning(pop))
    #define DLT_DISABLE_MISSING_RETURN  __pragma(warning(disable : 4715))

    #define DLT_FORCE_INLINE            __forceinline
#elif defined(__clang__)
    #define DLT_DISABLE_WARNING_PUSH    _Pragma("clang diagnostic push")
    #define DLT_DISABLE_WARNING_POP     _Pragma("clang diagnostic pop")
    #define DLT_DISABLE_MISSING_RETURN  _Pragma("clang diagnostic ignored \"-Wreturn-type\"")

    #define DLT_FORCE_INLINE __attribute__((always_inline))
#elif defined(__GNUC__) || defined(__GNUG__)
    #define DLT_DISABLE_WARNING_PUSH    _Pragma("GCC diagnostic push")
    #define DLT_DISABLE_WARNING_POP     _Pragma("GCC diagnostic pop")
    #define DLT_DISABLE_MISSING_RETURN  _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")

    #define DLT_FORCE_INLINE __attribute__((always_inline))
#else
    #define DLT_DISABLE_WARNING_PUSH  
    #define DLT_DISABLE_WARNING_POP   
    #define DLT_DISABLE_MISSING_RETURN

    #define DLT_FORCE_INLINE
#endif
