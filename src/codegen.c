#include <stdint.h>

#include "codegen.h"
#include "target.h"

#define code_emit_u8(g, val) da_append(&(g)->code, (val))
#define code_emit_str(g, cstr_lit) da_append_many(&(g)->code, (cstr_lit), sizeof(cstr_lit)-1)

static inline void code_emit_u32(Codegen *g, uint32_t val)
{
    code_emit_u8(g, (uint8_t)(val >> 0));
    code_emit_u8(g, (uint8_t)(val >> 8));
    code_emit_u8(g, (uint8_t)(val >> 16));
    code_emit_u8(g, (uint8_t)(val >> 24));
}

static inline void code_emit_u64(Codegen *g, uint64_t val)
{
    code_emit_u8(g, (uint8_t)(val >> 0));
    code_emit_u8(g, (uint8_t)(val >> 8));
    code_emit_u8(g, (uint8_t)(val >> 16));
    code_emit_u8(g, (uint8_t)(val >> 24));
    code_emit_u8(g, (uint8_t)(val >> 32));
    code_emit_u8(g, (uint8_t)(val >> 40));
    code_emit_u8(g, (uint8_t)(val >> 48));
    code_emit_u8(g, (uint8_t)(val >> 56));
}

static void code_emit_right(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            code_emit_u8(g, '\x48');

            // add rbx, operand
            if (operand <= 127) {
                code_emit_str(g, "\x83\xC3");
                code_emit_u8(g, (uint8_t)operand);
            } else {
                code_emit_str(g, "\x81\xC3");
                code_emit_u32(g, (uint32_t)operand);
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_left(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            code_emit_u8(g, '\x48');

            // sub rbx, operand
            if (operand <= 127) {
                code_emit_str(g, "\x83\xEB");
                code_emit_u8(g, (uint8_t)operand);
            } else {
                code_emit_str(g, "\x81\xEB");
                code_emit_u32(g, (uint32_t)operand);
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}
static void code_emit_add(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // add byte[rbx], operand
            code_emit_str(g, "\x80\x03");
            code_emit_u8(g, (uint8_t)operand);
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_sub(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // sub byte[rbx], operand
            code_emit_str(g, "\x80\x2B");
            code_emit_u8(g, (uint8_t)operand);
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_put(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            switch (get_target_os()) {
                case OS_WINDOWS: {
                    // sub rsp, 32
                    code_emit_str(g, "\x48\x83\xEC\x20");
                } break;
                default: break;
            }

            for (size_t i = 0; i < operand; i++) {
                switch (get_target_os()) {
                    case OS_WINDOWS: {
                        // movzx rcx, byte[rbx]
                        code_emit_str(g,"\x48\x0F\xB6\x0B");
                    } break;
                    default: {
                        // movzx rdi, byte[rbx]
                        code_emit_str(g, "\x48\x0F\xB6\x3B");
                    } break;
                }

                // mov rax, dummy
                code_emit_str(g, "\x48\xB8");
                da_append(&g->calls, ((Call){CALL_PUT, g->code.count}));

                // call rax
                code_emit_u64(g, 0);
                code_emit_str(g, "\xFF\xD0");
            }

            switch (get_target_os()) {
                case OS_WINDOWS: {
                    // add rsp, 32
                    code_emit_str(g, "\x48\x83\xC4\x20");
                } break;
                default: break;
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_get(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            switch (get_target_os()) {
                case OS_WINDOWS: {
                    // sub rsp, 32
                    code_emit_str(g, "\x48\x83\xEC\x20");
                } break;
                default: break;
            }

            for (size_t i = 0; i < operand; i++) {
                // mov rax, dummy
                code_emit_str(g, "\x48\xB8");
                da_append(&g->calls, ((Call){CALL_GET, g->code.count}));
                code_emit_u64(g, 0);

                // call rax
                code_emit_str(g, "\xFF\xD0");

                // mov byte[rbx], al
                code_emit_str(g, "\x88\x03");
            }

            switch (get_target_os()) {
                case OS_WINDOWS: {
                    // add rsp, 32
                    code_emit_str(g, "\x48\x83\xC4\x20");
                } break;
                default: break;
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_jz(Codegen *g)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // cmp byte[rbx], 0
            code_emit_str(g, "\x80\x3B\x00");

            // jz dummy
            code_emit_str(g, "\x0F\x84");
            da_append(&g->bp, g->code.count);
            code_emit_u32(g, 0);
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_jnz(Codegen *g)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // jmp
            code_emit_u8(g, '\xE9');

            int32_t jz_op_addr = (int32_t)da_pop(&g->bp);
            int32_t jnz_op_addr = (int32_t)g->code.count;

            int32_t jz_op = (jnz_op_addr + 4) - (jz_op_addr + 4);
            int32_t jnz_op = (jz_op_addr - 5) - (jnz_op_addr + 4);

            memcpy(g->code.items + jz_op_addr, &jz_op, sizeof(jz_op));
            code_emit_u32(g, jnz_op);
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_zero(Codegen *g)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // mov byte[rbx], 0
            code_emit_str(g, "\xC6\x03\x00");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

Codegen generate_code(Bf_IRs *irs)
{
    Codegen g = {0};

    switch (get_target_arch()) {
        case ARCH_X64: {
            // push rbx
            code_emit_u8(&g, '\x53');
            switch (get_target_os()) {
                case OS_WINDOWS:
                    // mov rbx, rcx
                    code_emit_str(&g, "\x48\x89\xCB");
                    break;
                default:
                    // mov rbx, rdi
                    code_emit_str(&g, "\x48\x89\xFB");
                    break;
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }

    for (size_t i = 0; i < irs->count; i++) {
        switch (irs->items[i].kind) {
            case IR_RIGHT:
                code_emit_right(&g, irs->items[i].operand);
                break;
            case IR_LEFT:
                code_emit_left(&g, irs->items[i].operand);
                break;
            case IR_ADD:
                code_emit_add(&g, irs->items[i].operand);
                break;
            case IR_SUB:
                code_emit_sub(&g, irs->items[i].operand);
                break;
            case IR_PUT:
                code_emit_put(&g, irs->items[i].operand);
                break;
            case IR_GET:
                code_emit_get(&g, irs->items[i].operand);
                break;
            case IR_JZ:
                code_emit_jz(&g);
                break;
            case IR_JNZ:
                code_emit_jnz(&g);
                break;
            case IR_ZERO:
                code_emit_zero(&g);
                break;
            default: basic_unreachable();
        }
    }

    switch (get_target_arch()) {
        case ARCH_X64: {
            // pop rbx
            code_emit_u8(&g, '\x5B');
            // ret
            code_emit_u8(&g, '\xC3');
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }

    da_free(&g.bp);
    return g;
}

void code_free(Codegen *g)
{
    da_free(&g->calls);
    sb_free(&g->code);
}
