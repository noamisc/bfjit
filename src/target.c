#include "target.h"

Os get_target_os(void)
{
#if defined(_WIN32)
    return OS_WINDOWS;
#elif defined(__APPLE__)
    return OS_MACOS;
#elif defined(__linux__)
    return OS_LINUX;
#else
    return OS_UNKNOWN;
#endif
}

Arch get_target_arch(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return ARCH_X64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return ARCH_AARCH64;
#else
    return ARCH_UNKNOWN;
#endif
}
