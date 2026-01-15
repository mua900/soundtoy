#pragma once

#include <cstdint>

#if    defined(__amd64__) || defined(_M_X64) || defined(__x86_64__) || defined(__amd64) || defined(__x86_64) \
    || defined(__i386__)  || defined(_M_X86) || defined(__x86__)    || defined(__i386)  || defined(__x86)
    #define ARCH_IS_X86   1
#else
    #define ARCH_IS_X86   0
#endif

#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    #define ARCH_IS_ARM64 1
#else
    #define ARCH_IS_ARM64 0
#endif

#if (defined(__arm__) || defined(_M_ARM)) && !ARCH_IS_X64
    #define ARCH_IS_ARM32 1
#else
    #define ARCH_IS_ARM32 0
#endif

#if   ARCH_IS_X86
    #include <xmmintrin.h>
    #include <pmmintrin.h>
#endif

#define ARCH_IS_64_BITS (sizeof(void*) == 8)
#define ARCH_IS_32_BITS (sizeof(void*) == 4)

static_assert(ARCH_IS_32_BITS || ARCH_IS_64_BITS);

void enable_flush_denormals();