#ifndef BFJIT_CODEGEN_H_
#define BFJIT_CODEGEN_H_

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
} Calls;

typedef struct {
    String_Builder code;
    Bracket_Pos bp;
    Calls calls;
} Codegen;

Codegen generate_code(Bf_IRs *irs);
void code_free(Codegen *g);

#endif // BFJIT_CODEGEN_H_
