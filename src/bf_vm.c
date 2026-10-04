#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

#include "bf_vm.h"

static inline bool jit_memory_invalid(Jit_Memory *jit_mem)
{
#ifdef _WIN32
    return jit_mem->data == NULL;
#else
    return jit_mem->data == MAP_FAILED;
#endif
}

static inline bool jit_memory_alloc(Jit_Memory *jit_mem)
{
#ifdef _WIN32
    jit_mem->data = VirtualAlloc(NULL, jit_mem->size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    jit_mem->data = mmap(NULL, jit_mem->size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    return !jit_memory_invalid(jit_mem);
}

static inline bool jit_memory_dealloc(Jit_Memory *jit_mem)
{
#ifdef _WIN32
    return VirtualFree(jit_mem->data, 0, MEM_RELEASE) != 0;
#else
    return munmap(jit_mem->data, jit_mem->size) == 0;
#endif
}

static inline bool bf_tape_alloc(Bf_Tape *tape)
{
    tape->data = calloc(tape->size, 1);
    return tape->data != NULL;
}

static inline void bf_tape_dealloc(Bf_Tape *tape)
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

void bf_vm_deinit(Bf_VM *vm)
{
    if (vm->codegen.call_stack.items) da_free(&vm->codegen.call_stack);
    if (vm->codegen.code.items) sb_free(&vm->codegen.code);
    if (!jit_memory_invalid(&vm->jit_mem)) jit_memory_dealloc(&vm->jit_mem);
    if (vm->tape.data) bf_tape_dealloc(&vm->tape);

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
        }
    }

    da_free(&bp);
}

bool bf_vm_interpret(Bf_VM *vm)
{
    if (!bf_tape_alloc(&vm->tape)) {
        return false;
    }

    bf_vm_match_bracket(vm);

    uint8_t *ptr = vm->tape.data;

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
        }
    }


    bf_tape_dealloc(&vm->tape);
    return true;
}

/*
  // old

  bool bf_vm_jit_compile(Bf_VM *vm)
{
    bool result = true;

    vm->codegen = generate_code(vm->irs);
    vm->jit_mem.size = vm->codegen.code.count;

    if (!jit_memory_alloc(&vm->jit_mem)) {
        basic_return_defer(false);
    }

    if (!bf_tape_alloc(&vm->tape)) {
        basic_return_defer(false);
    }

    memcpy(vm->jit_mem.data, vm->codegen.code.items, vm->codegen.code.count);

    for (size_t i = 0; i < vm->codegen.call_stack.count; i++) {
        size_t pos = vm->codegen.call_stack.items[i].pos;
        uint8_t *next = vm->jit_mem.data + pos + 4;
        int32_t offset;

        switch (vm->codegen.call_stack.items[i].kind) {
            case CALL_PUT:
                offset = (int32_t)((intptr_t)putchar - (intptr_t)next);
                break;
            case CALL_GET:
                offset = (int32_t)((intptr_t)getchar - (intptr_t)next);
                break;
        }

        memcpy(vm->jit_mem.data + pos, &offset, sizeof(offset));
    }

    void (*program)(void *) = vm->jit_mem.data;
    program(vm->tape.data);

defer:
    if (!jit_memory_invalid(&vm->jit_mem)) jit_memory_dealloc(&vm->jit_mem);
    if (vm->tape.data) bf_tape_dealloc(&vm->tape);
    return result;
}
 */

bool bf_vm_jit_compile(Bf_VM *vm)
{
    bool result = true;

    vm->codegen = generate_code(vm->irs);
    vm->jit_mem.size = vm->codegen.code.count;

    if (!jit_memory_alloc(&vm->jit_mem)) {
        basic_return_defer(false);
    }

    if (!bf_tape_alloc(&vm->tape)) {
        basic_return_defer(false);
    }

    memcpy(vm->jit_mem.data, vm->codegen.code.items, vm->codegen.code.count);

    for (size_t i = 0; i < vm->codegen.call_stack.count; i++) {
        Call call = vm->codegen.call_stack.items[i];
        uintptr_t fn;

        switch (call.kind) {
            case CALL_PUT:
                fn = (uintptr_t)putchar;
                break;
            case CALL_GET:
                fn = (uintptr_t)getchar;
                break;
        }

        memcpy(vm->jit_mem.data + call.pos, &fn, sizeof(fn));
    }

    void (*program)(void *) = vm->jit_mem.data;
    program(vm->tape.data);

defer:
    return result;
}
