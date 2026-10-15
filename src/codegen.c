#include <stdint.h>

#include "codegen.h"
#include "target.h"

#define code_emit_u8(g, val) da_append(&(g)->code, (val))
#define code_emit_str(g, cstr_lit) da_append_many(&(g)->code, (cstr_lit), sizeof(cstr_lit)-1)

static inline void code_emit_u32(Codegen *g, uint32_t val)
{
    code_emit_u8(g, (uint8_t)(val));
    code_emit_u8(g, (uint8_t)(val >> 8));
    code_emit_u8(g, (uint8_t)(val >> 16));
    code_emit_u8(g, (uint8_t)(val >> 24));
}

static inline void code_emit_u64(Codegen *g, uint64_t val)
{
    code_emit_u32(g, (uint32_t)val);
    code_emit_u32(g, (uint32_t)(val >> 32));
}

static void code_emit_right(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // add rbx, operand
            if (operand <= 127) {
                code_emit_str(g, "\x48\x83\xC3");
                code_emit_u8(g, (uint8_t)operand);
            } else {
                while (operand != 0) {
                    uint32_t val = (operand > INT32_MAX) ? INT32_MAX : (uint32_t)operand;
                    code_emit_str(g, "\x48\x81\xC3");
                    code_emit_u32(g, val);
                    operand -= val;
                }
            }
        } break;
        case ARCH_ARM64: {
            uint32_t inst = 0x91000273; // add x19, x19, #0

            while (operand != 0) {
                uint32_t val = (operand > 4075) ? 4075 : (uint32_t)operand;
                code_emit_u32(g, inst | (val << 10));
                operand -= val;
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_left(Codegen *g, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // sub rbx, operand
            if (operand <= 127) {
                code_emit_str(g, "\x48\x83\xEB");
                code_emit_u8(g, (uint8_t)operand);
            } else {
                while (operand != 0) {
                    uint32_t val = (operand > INT32_MAX) ? INT32_MAX : (uint32_t)operand;
                    code_emit_str(g, "\x48\x81\xEB");
                    code_emit_u32(g, val);
                    operand -= val;
                }
            }
        } break;
        case ARCH_ARM64: {
            uint32_t inst = 0xD1000273; // sub x19, x19, #0

            while (operand != 0) {
                uint32_t val = (operand > 4095) ? 4095 : (uint32_t)operand;
                code_emit_u32(g, inst | (val << 10));
                operand -= val;
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
        case ARCH_ARM64: {
            // ldrb w0, [x19]
            code_emit_str(g, "\x60\x02\x40\x39");

            // add w0, w0, #operand
            code_emit_u32(g, 0x11000000 | ((uint32_t)(operand & 0xFFF) << 10));

            // strb w0, [x19]
            code_emit_str(g, "\x60\x02\x00\x39");
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
        case ARCH_ARM64: {
            // ldrb w0, [x19]
            code_emit_str(g, "\x60\x02\x40\x39");

            // sub w0, w0, #operand
            code_emit_u32(g, 0x51000000 | ((uint32_t)(operand & 0xFFF) << 10));

            // strb w0, [x19]
            code_emit_str(g, "\x60\x02\x00\x39");
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
                code_emit_u64(g, 0);

                // call rax
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
        case ARCH_ARM64: {
            // ldrb w0, [x19]
            code_emit_str(g, "\x60\x02\x40\x39");

            // ldr x8, #12
            code_emit_str(g, "\x68\x00\x00\x58");

            // blr x8
            code_emit_str(g, "\x00\x01\x3F\xD6");

            // b #12
            code_emit_str(g, "\x03\x00\x00\x14");

            da_append(&g->calls, ((Call){CALL_PUT, g->code.count}));
            code_emit_u64(g, 0);
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
        case ARCH_ARM64: {
            // ldr x8, #12
            code_emit_str(g, "\x68\x00\x00\x58");

            // blr x8
            code_emit_str(g, "\x00\x01\x3F\xD6");

            // b #12
            code_emit_str(g, "\x03\x00\x00\x14");

            da_append(&g->calls, ((Call){CALL_GET, g->code.count}));
            code_emit_u64(g, 0);

            // strb w0, [x19]
            code_emit_str(g, "\x60\x02\x00\x39");
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
        case ARCH_ARM64: {
            // ldrb w0, [x19]
            code_emit_str(g, "\x60\x02\x40\x39");

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
        case ARCH_ARM64: {
            int32_t jz_addr = (int32_t)da_pop(&g->bp);
            int32_t jnz_addr = (int32_t)g->code.count;

            uint32_t jz_op = 0x34000000 | (((uint32_t)((jnz_addr + 4) - jz_addr) >> 2) & 0x7FFFF) << 5;
            uint32_t jnz_op = 0x14000000 | (((uint32_t)((jz_addr - 4) - jnz_addr) >> 2) & 0x3FFFFFF);

            memcpy(g->code.items + jz_addr, &jz_op, sizeof(jz_op));
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
        case ARCH_ARM64: {
            // strb wzr, [x19]
            code_emit_str(g, "\x7F\x02\x00\x39");
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
        case ARCH_ARM64: {
            // stp  x19, x30 [sp, #-16]!
            code_emit_str(&g, "\xF3\x7B\xBF\xA9");
            // mov x19, x0
            code_emit_str(&g, "\xF3\x03\x00\xAA");
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
        case ARCH_ARM64: {
            // ldp x19, x30, [sp], #16
            code_emit_str(&g, "\xF3\x7B\xC1\xA8");
            // ret
            code_emit_str(&g, "\xC0\x03\x5F\xD6");
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
