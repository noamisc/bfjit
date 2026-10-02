#ifndef VM_H_
#define VM_H_

#include "bf_ir.h"
#include "codegen.h"

typedef struct {
    void *data;
    size_t size;
} Jit_Memory;

typedef struct {
    void *data;
    size_t size;
} Bf_Tape;

typedef struct {
    Bf_IRs *irs;
    Codegen codegen;
    Jit_Memory jit_mem;
    Bf_Tape tape;
} Bf_VM;

Bf_VM bf_vm_init(Bf_IRs *irs);
bool bf_vm_interpret(Bf_VM *vm);
bool bf_vm_jit_compile(Bf_VM *vm);

#endif // VM_H_
