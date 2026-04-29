#pragma once

#ifdef _MSC_VER
    #define DLT_FORCE_INLINE __forceinline
#else
    #define DLT_FORCE_INLINE __attribute__((always_inline))
#endif
