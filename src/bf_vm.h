#ifndef BFJIT_VM_H_
#define BFJIT_VM_H_

#include "bf_ir.h"
#include "codegen.h"

typedef struct {
    void *data;
    size_t size;
} Bf_Code;

typedef struct {
    uint8_t *data;
    size_t size;
} Bf_Tape;

typedef struct {
    Bf_IRs *irs;
    Codegen g;
    Bf_Code code;
    Bf_Tape tape;
} Bf_VM;

Bf_VM bf_vm_init(Bf_IRs *irs);

bool bf_vm_interpret(Bf_VM *vm);
bool bf_vm_jit_compile(Bf_VM *vm);

#endif // BFJIT_VM_H_
