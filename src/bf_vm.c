#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32)
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#endif

#include "bf_vm.h"

typedef void (*Bf_Program)(void *);

static inline bool bf_code_invalid(Bf_Code *code)
{
#ifdef _WIN32
    return code->data == NULL;
#else
    return code->data == MAP_FAILED;
#endif
}

static inline bool bf_code_alloc(Bf_Code *code)
{
#if defined(_WIN32)
    code->data = VirtualAlloc(NULL, code->size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined(__APPLE__)
    code->data = mmap(NULL, code->size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
#else
    code->data = mmap(NULL, code->size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

    return !bf_code_invalid(code);
}

static inline bool bf_code_free(Bf_Code *code)
{
#if defined(_WIN32)
    return VirtualFree(code->data, 0, MEM_RELEASE) != 0;
#else
    return munmap(code->data, code->size) == 0;
#endif
}

static inline bool bf_tape_alloc(Bf_Tape *tape)
{
    tape->data = calloc(tape->size, sizeof(*tape->data));
    return tape->data != NULL;
}

static inline void bf_tape_free(Bf_Tape *tape)
{
    free(tape->data);
}

Bf_VM bf_vm_init(Bf_IRs *irs)
{
    return (Bf_VM) {
        .irs = irs,
        .tape = {
            .size = 30000
        },
    };
}

static inline void bf_vm_deinit(Bf_VM *vm)
{
    code_free(&vm->g);
    bf_code_free(&vm->code);
    bf_tape_free(&vm->tape);
}

// used by the interpreter
static void bf_vm_match_bracket(Bf_VM *vm)
{
    Bracket_Pos bp = {0};

    for (size_t i = 0; i < vm->irs->count; i++) {
        switch (vm->irs->items[i].kind) {
            case IR_JZ:
                da_push(&bp, i);
                break;
            case IR_JNZ:
                vm->irs->items[i].operand = da_peek_last(&bp);
                vm->irs->items[da_pop(&bp)].operand = i;
                break;
            default: break;
        }
    }

    da_free(&bp);
}

bool bf_vm_interpret(Bf_VM *vm)
{
    if (!bf_tape_alloc(&vm->tape)) {
        return false;
    }

    uint8_t *ptr = vm->tape.data;
    bf_vm_match_bracket(vm);

    for (size_t pc = 0; pc < vm->irs->count; pc++) {
        switch (vm->irs->items[pc].kind) {
            case IR_RIGHT:
                ptr += vm->irs->items[pc].operand;
                break;
            case IR_LEFT:
                ptr -= vm->irs->items[pc].operand;
                break;
            case IR_ADD:
                *ptr += vm->irs->items[pc].operand;
                break;
            case IR_SUB:
                *ptr -= vm->irs->items[pc].operand;
                break;
            case IR_PUT:
                for (size_t i = 0; i < vm->irs->items[pc].operand; i++) {
                    putchar(*ptr);
                }
                break;
            case IR_GET:
                for (size_t i = 0; i < vm->irs->items[pc].operand; i++) {
                    *ptr = getchar();
                }
                break;
            case IR_JZ: {
                if (*ptr == 0) {
                    pc = vm->irs->items[pc].operand;
                }
            } break;
            case IR_JNZ: {
                if (*ptr != 0) {
                    pc = vm->irs->items[pc].operand;
                }
            } break;
            case IR_ZERO:
                *ptr = 0;
                break;
            default: basic_unreachable();
        }
    }

    return true;
}

bool bf_vm_jit_compile(Bf_VM *vm)
{
    bool result = true;

    vm->g = generate_code(vm->irs);
    vm->code.size = vm->g.code.count;

    if (!bf_code_alloc(&vm->code)) {
        basic_return_defer(false);
    }

    if (!bf_tape_alloc(&vm->tape)) {
        basic_return_defer(false);
    }

#if defined(__APPLE__)
    pthread_jit_write_protect_np(0);
#endif

    memcpy(vm->code.data, vm->g.code.items, vm->g.code.count);

    for (size_t i = 0; i < vm->g.calls.count; i++) {
        uintptr_t fn;

        switch (vm->g.calls.items[i].kind) {
            case CALL_PUT:
                fn = (uintptr_t)putchar;
                break;
            case CALL_GET:
                fn = (uintptr_t)getchar;
                break;
            default: basic_unreachable();
        }

        memcpy((uint8_t *)vm->code.data + vm->g.calls.items[i].pos, &fn, sizeof(fn));
    }

#if defined(__APPLE__)
    pthread_jit_write_protect_np(1);
    sys_icache_invalidate(vm->code.data, vm->code.size);
#endif

    Bf_Program program = (Bf_Program)vm->code.data;
    program(vm->tape.data);

defer:
    bf_vm_deinit(vm);
    return result;
}
