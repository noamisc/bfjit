#ifndef IR_H_
#define IR_H_

#include "basic.h"
#include "error.h"

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} Bracket_Pos;

typedef enum {
    IR_NONE,
    IR_RIGHT = '>',
    IR_LEFT = '<',
    IR_ADD = '+',
    IR_SUB = '-',
    IR_PUT = '.',
    IR_GET = ',',
    IR_JZ = '[',
    IR_JNZ = ']',
    IR_ZERO
} Bf_IR_Kind;

typedef struct {
    Bf_IR_Kind kind;
    size_t operand;
    // for error handling
    size_t pos;
} Bf_IR;

typedef struct {
    String_View source;
    Bf_IR *items;
    size_t count;
    size_t capacity;
} Bf_IRs;

Bf_IRs generate_bf_irs(String_View source);
bool parse_bf_irs(Bf_IRs *irs, Errors *errors);
Bf_IRs optimize_bf_irs(Bf_IRs *irs);

// for debugging purposes
void dump_bf_irs(Bf_IRs *irs);

#endif // IR_H_
