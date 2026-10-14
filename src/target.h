#ifndef BFJIT_TARGET_H_
#define BFJIT_TARGET_H_

typedef enum {
    OS_UNKNOWN,
    OS_WINDOWS,
    OS_MACOS,
    OS_LINUX
} Os;

typedef enum {
    ARCH_UNKNOWN,
    ARCH_X64,
    ARCH_ARM64
} Arch;

Os get_target_os(void);
Arch get_target_arch(void);

#endif // BFJIT_TARGET_H_
