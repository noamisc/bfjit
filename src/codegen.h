#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "basic.h"
#include "bf_ir.h"

typedef enum {
    CALL_PUT,
    CALL_GET
} Call_Kind;

typedef struct {
    Call_Kind kind;
    size_t pos;
} Call;

typedef struct {
    Call *items;
    size_t count;
    size_t capacity;
} Call_Stack;

typedef struct {
    String_Builder code;
    Call_Stack call_stack;
    Bracket_Pos bracket_pos;
} Codegen;

Codegen generate_code(Bf_IRs *irs);

#endif // CODEGEN_H_
