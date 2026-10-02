#include <stdio.h>

#include "bf_ir.h"

typedef struct {
    String_View source;
    size_t pos;
    Bf_IRs irs;
} Lexer;

static inline void lexer_append(Lexer *lexer, Bf_IR_Kind kind, size_t operand, size_t pos)
{
    da_append(&lexer->irs, ((Bf_IR){kind, operand, pos}));
}

Bf_IRs generate_bf_irs(String_View source)
{
    Lexer lexer = {
        .source = source,
        .irs = {
            .source = source
        }
    };

    while (lexer.pos < lexer.source.count) {
        switch (lexer.source.data[lexer.pos]) {
            case '>':
            case '<':
            case '+':
            case '-':
            case '.':
            case ',':
            case '[':
            case ']':
                lexer_append(&lexer, lexer.source.data[lexer.pos], 1, lexer.pos);
                break;
            default: break;
        }

        lexer.pos += 1;
    }

    return lexer.irs;
}

bool parse_bf_irs(Bf_IRs *irs, Errors *errors)
{
    bool result = true;
    Bracket_Pos bp = {0};

    errors->source = irs->source;

    for (size_t i = 0; i < irs->count; i++) {
        switch (irs->items[i].kind) {
            case IR_JZ:
                da_push(&bp, irs->items[i].pos);
                break;
            case IR_JNZ:
                if (!da_empty(&bp)) {
                    da_pop(&bp);
                } else {
                    da_append(errors, ((Error){"unexpected closing bracket ']'", irs->items[i].pos}));
                    result = false;
                }
                break;
            default: break;
        }
    }

    for (size_t i = 0; i < bp.count; i++) {
        da_append(errors, ((Error){"unclosed bracket '['", bp.items[i]}));
        result = false;
    }

    da_free(&bp);
    return result;
}

static inline bool bf_ir_kind_groupable(Bf_IR_Kind kind)
{
    switch (kind) {
        case IR_RIGHT:
        case IR_LEFT:
        case IR_ADD:
        case IR_SUB:
        case IR_PUT:
        case IR_GET:
            return true;
        default: return false;
    }
}

static inline Bf_IR_Kind bf_ir_kind_opposite(Bf_IR_Kind kind)
{
    switch (kind) {
        case IR_RIGHT: return IR_LEFT;
        case IR_LEFT: return IR_RIGHT;
        case IR_ADD: return IR_SUB;
        case IR_SUB: return IR_ADD;
        default: return IR_NONE;
    }
}

Bf_IRs optimize_bf_irs(Bf_IRs *irs)
{
    Bf_IRs result = {0};

    if (irs->count > 0) {
        da_append(&result, irs->items[0]);

        for (size_t i = 1; i < irs->count; i++) {
            Bf_IR current = irs->items[i];
            Bf_IR *last = &result.items[result.count-1];

            if (current.kind == last->kind && bf_ir_kind_groupable(current.kind)) {
                last->operand += current.operand;
            } else if (current.kind == bf_ir_kind_opposite(last->kind)) {
                if (current.operand < last->operand) {
                    last->operand -= current.operand;
                } else {
                    if (current.operand - last->operand != 0) {
                        last->kind = current.kind;
                        last->operand = current.operand - last->operand;
                    } else {
                        da_pop(&result);
                    }
                }
            } else if (result.count >= 2 && result.items[result.count-2].kind == IR_JZ && ((result.items[result.count-1].kind == IR_ADD || result.items[result.count-1].kind == IR_SUB) && result.items[result.count-1].operand == 1) && current.kind == IR_JNZ) {
                da_pop(&result);
                result.items[result.count-1].kind = IR_ZERO;
            } else {
                da_append(&result, current);
            }
        }
    }

    return result;
}

void dump_bf_irs(Bf_IRs *irs)
{
    for (size_t i = 0; i < irs->count; i++) {
        if (irs->items[i].kind == IR_ZERO) {
            printf("{\"[-]\", %zu}", irs->items[i].operand);
        } else {
            printf("{\"%c\", %zu}", irs->items[i].kind, irs->items[i].operand);
        }
        putchar('\n');
    }
}
