#include "platform.h"

void enable_flush_denormals() {
#if   ARCH_IS_X86
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#elif ARCH_IS_ARM64
    // @todo
    uint64_t fpcr;
#elif ARCH_IS_ARM32
    // @todo
    uint32_t fpcr;
#endif
}
