#include <stdint.h>

#include "codegen.h"
#include "target.h"

#define code_emit_u8(code, val) da_append(code, val)
#define code_emit_str(code, cstr_lit) da_append_many(code, cstr_lit, sizeof(cstr_lit)-1)

static inline void code_emit_u32(String_Builder *code, uint32_t val)
{
    code_emit_u8(code, (uint8_t)(val >> 0));
    code_emit_u8(code, (uint8_t)(val >> 8));
    code_emit_u8(code, (uint8_t)(val >> 16));
    code_emit_u8(code, (uint8_t)(val >> 24));
}

static inline void code_emit_u64(String_Builder *code, uint64_t val)
{
    code_emit_u8(code, (uint8_t)(val >> 0));
    code_emit_u8(code, (uint8_t)(val >> 8));
    code_emit_u8(code, (uint8_t)(val >> 16));
    code_emit_u8(code, (uint8_t)(val >> 24));
    code_emit_u8(code, (uint8_t)(val >> 32));
    code_emit_u8(code, (uint8_t)(val >> 40));
    code_emit_u8(code, (uint8_t)(val >> 48));
    code_emit_u8(code, (uint8_t)(val >> 56));
}

static void code_emit_right(Codegen *codegen, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
                code_emit_u8(&codegen->code, '\x48');
                // add rbx, operand
                if (operand <= 127) {
                    code_emit_str(&codegen->code, "\x83\xC3");
                    code_emit_u8(&codegen->code, (uint8_t)operand);
                } else {
                    code_emit_str(&codegen->code, "\x81\xC3");
                    code_emit_u32(&codegen->code, (uint32_t)operand);
                }
        } break;
        case ARCH_AARCH64: {
            while (operand > 4095) {
                // add x19, x19, #4095
                code_emit_str(&codegen->code, "\x91\x3f\xfe\x73");
                operand -= 4095;
            }

            if (operand != 0) {
                // add x19, x19, #operand
                uint32_t inst = 0x91000273 | (uint32_t)operand << 10;
                code_emit_u32(&codegen->code, inst);
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_left(Codegen *codegen, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            code_emit_u8(&codegen->code, '\x48');

            // sub rbx, operand
            if (operand <= 127) {
                code_emit_str(&codegen->code, "\x83\xEB");
                code_emit_u8(&codegen->code, (uint8_t)operand);
            } else {
                code_emit_str(&codegen->code, "\x81\xEB");
                code_emit_u32(&codegen->code, (uint32_t)operand);
            }
        } break;
        case ARCH_AARCH64: {
            while (operand > 4095) {
                // sub x19, x19, #4095
                code_emit_str(&codegen->code, "\xd1\x3f\xfe\x73");
                operand -= 4095;
            }

            if (operand != 0) {
                // sub x19, x19, #operand
                uint32_t inst = 0xd1000273 | (uint32_t)operand << 10;
                code_emit_u32(&codegen->code, inst);
            }
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_add(Codegen *codegen, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // add byte[rbx], operand
            code_emit_str(&codegen->code, "\x80\x03");
            code_emit_u8(&codegen->code, (uint8_t)operand);
        } break;
        case ARCH_AARCH64: {
            // ldrb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x40\x02\x60");

            while (operand > 4095) {
                // add w0, w0, #4095
                code_emit_str(&codegen->code, "\x11\x3f\xfc\x00");
                operand -= 4095;
            }

            if (operand != 0) {
                // add w0, w0, #operand
                uint32_t inst = 0x11000000 | (uint32_t)operand << 10;
                code_emit_u32(&codegen->code, inst);
            }

            // strb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x00\x02\x60");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_sub(Codegen *codegen, size_t operand)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // sub byte[rbx], operand
            code_emit_str(&codegen->code, "\x80\x2B");
            code_emit_u8(&codegen->code, (uint8_t)operand);
        } break;
        case ARCH_AARCH64: {
            // ldrb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x40\x02\x60");

            while (operand > 4095) {
                // sub w0, w0, #4095
                code_emit_str(&codegen->code, "\x51\x3f\xfc\x00");
                operand -= 4095;
            }

            if (operand != 0) {
                // sub w0, w0, #operand
                uint32_t inst = 0x51000000 | (uint32_t)operand << 10;
                code_emit_u32(&codegen->code, inst);
            }

            // strb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x00\x02\x60");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_put(Codegen *codegen, size_t operand)
{
    for (size_t i = 0; i < operand; i++) {
        switch (get_target_arch()) {
            case ARCH_X64: {
                switch (get_target_os()) {
                    case OS_WINDOWS: {
                        // movzx rcx, byte[rbx]
                        code_emit_str(&codegen->code, "\x48\x0F\xB6\x0B");
                        // sub rsp, 32
                        code_emit_str(&codegen->code, "\x48\x83\xEC\x20");
                    } break;
                    default: {
                        // movzx rdi, byte[rbx]
                        code_emit_str(&codegen->code, "\x48\x0F\xB6\x3B");
                    } break;
                }

                // mov rax, dummy
                code_emit_str(&codegen->code, "\x48\xB8");
                da_append(&codegen->call_stack, ((Call){CALL_PUT, codegen->code.count}));
                // call rax
                code_emit_u64(&codegen->code, 0);
                code_emit_str(&codegen->code, "\xFF\xD0");

                switch (get_target_os()) {
                    case OS_WINDOWS: {
                        // add rsp, 32
                        code_emit_str(&codegen->code, "\x48\x83\xC4\x20");
                    } break;
                    default: break;
                }
            } break;
            case ARCH_AARCH64: {
                // ldrb w0, [x19]
                code_emit_str(&codegen->code, "\x39\x40\x02\x60");
                // ldr x8, #8
                code_emit_str(&codegen->code, "\x58\x00\x00\x48");
                // blr x8
                code_emit_str(&codegen->code, "\xd6\x3f\x01\x00");

                da_append(&codegen->call_stack, ((Call){CALL_PUT, codegen->code.count}));
                code_emit_u64(&codegen->code, 0);
            } break;
            default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
        }
    }
}

static void code_emit_get(Codegen *codegen, size_t operand)
{
    for (size_t i = 0; i < operand; i++) {
        switch (get_target_arch()) {
            case ARCH_X64: {
                switch (get_target_os()) {
                    case OS_WINDOWS: {
                        // sub rsp, 32
                        code_emit_str(&codegen->code, "\x48\x83\xEC\x20");
                    } break;
                    default: break;
                }

                // mov rax, dummy
                code_emit_str(&codegen->code, "\x48\xB8");
                da_append(&codegen->call_stack, ((Call){CALL_GET, codegen->code.count}));
                code_emit_u64(&codegen->code, 0);
                // call rax
                code_emit_str(&codegen->code, "\xFF\xD0");

                switch (get_target_os()) {
                    case OS_WINDOWS: {
                        // add rsp, 32
                        code_emit_str(&codegen->code, "\x48\x83\xC4\x20");
                    } break;
                    default: break;
                }
            } break;
            case ARCH_AARCH64: {
                // ldrb w0, [x19]
                code_emit_str(&codegen->code, "\x39\x40\x02\x60");
                // ldr x8, #8
                code_emit_str(&codegen->code, "\x58\x00\x00\x48");
                // blr x8
                code_emit_str(&codegen->code, "\xd6\x3f\x01\x00");

                da_append(&codegen->call_stack, ((Call){CALL_GET, codegen->code.count}));
                code_emit_u64(&codegen->code, 0);

                // strb w0, [x19]
                code_emit_str(&codegen->code, "\x39\x00\x02\x60");
            } break;
            default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
        }
    }
}

static void code_emit_jz(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // cmp byte[rbx], 0
            code_emit_str(&codegen->code, "\x80\x3B\x00");
            // jz dummy
            code_emit_str(&codegen->code, "\x0F\x84");
            da_append(&codegen->bracket_pos, codegen->code.count);
            code_emit_u32(&codegen->code, 0);
        } break;
        case ARCH_AARCH64: {
            // ldrb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x40\x02\x60");
            // dummy
            da_append(&codegen->bracket_pos, codegen->code.count);
            code_emit_str(&codegen->code, "\x00\x00\x00\x00");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_jnz(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // cmp byte[rbx], 0
            code_emit_str(&codegen->code, "\x80\x3B\x00");
            // jnz
            code_emit_str(&codegen->code, "\x0F\x85");

            size_t jz_op_addr = da_pop(&codegen->bracket_pos);
            int32_t jnz_op_addr = (int32_t)codegen->code.count;

            int32_t jz_op = jnz_op_addr - (int32_t)jz_op_addr;
            int32_t jnz_op = (int32_t)((jz_op_addr - 2) - (jnz_op_addr + 4));

            code_emit_u32(&codegen->code, (uint32_t)jnz_op);
            memcpy(codegen->code.items + jz_op_addr, &jz_op, sizeof(jz_op));
        } break;
        case ARCH_AARCH64: {
            // ldrb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x40\x02\x60");

            size_t jz_addr = da_pop(&codegen->bracket_pos);
            int32_t jnz_addr = (int32_t)codegen->code.count;

            int32_t jz_op = jnz_addr - (int32_t)jz_addr + 4;
            int32_t jnz_op = (int32_t)jz_addr - jnz_addr + 4;

            // cbnz
            uint32_t jnz_inst = 0x35000000 | ((uint32_t)(jnz_op / 4) & 0x7FFFF) << 5;
            code_emit_u32(&codegen->code, jnz_inst);

            uint32_t jz_inst = 0x34000000 | ((uint32_t)(jz_op / 4) & 0x7FFFF) << 5;
            memcpy(codegen->code.items + jz_addr, &jz_inst, sizeof(jz_inst));
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_emit_zero(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // mov byte[rbx], 0
            code_emit_str(&codegen->code, "\xC6\x03\x00");
        } break;
        case ARCH_AARCH64: {
            // mov w0, #0
            code_emit_str(&codegen->code, "\x52\x80\x00\x00");
            // strb w0, [x19]
            code_emit_str(&codegen->code, "\x39\x00\x02\x60");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_init(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // push rbx
            code_emit_u8(&codegen->code, '\x53');
            switch (get_target_os()) {
                case OS_WINDOWS:
                    // mov rbx, rcx
                    code_emit_str(&codegen->code, "\x48\x89\xCB");
                    break;
                default:
                    // mov rbx, rdi
                    code_emit_str(&codegen->code, "\x48\x89\xFB");
                    break;
            }
        } break;
        case ARCH_AARCH64: {
            // str x19, [sp, #-16]!
            code_emit_str(&codegen->code, "\xF8\x1F\x0F\xF3");
            // mov x19, x0
            code_emit_str(&codegen->code, "\xAA\x00\x03\xF3");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

static void code_eof(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // pop rbx
            code_emit_u8(&codegen->code, '\x5B');
            // ret
            code_emit_u8(&codegen->code, '\xC3');
        } break;
        case ARCH_AARCH64: {
            // ldr x19, [sp], #16
            code_emit_str(&codegen->code, "\xF8\x41\x07\xF3");
            // ret
            code_emit_str(&codegen->code, "\xD6\x5F\x03\xC0");
        } break;
        default: basic_panic("Oops! it looks like bfjit doesn't support this architecture yet");
    }
}

Codegen generate_code(Bf_IRs *irs)
{
    Codegen codegen = {0};

    code_init(&codegen);

    for (size_t i = 0; i < irs->count; i++) {
        switch (irs->items[i].kind) {
            case IR_RIGHT:
                code_emit_right(&codegen, irs->items[i].operand);
                break;
            case IR_LEFT:
                code_emit_left(&codegen, irs->items[i].operand);
                break;
            case IR_ADD:
                code_emit_add(&codegen, irs->items[i].operand);
                break;
            case IR_SUB:
                code_emit_sub(&codegen, irs->items[i].operand);
                break;
            case IR_PUT:
                code_emit_put(&codegen, irs->items[i].operand);
                break;
            case IR_GET:
                code_emit_get(&codegen, irs->items[i].operand);
                break;
            case IR_JZ:
                code_emit_jz(&codegen);
                break;
            case IR_JNZ:
                code_emit_jnz(&codegen);
                break;
            case IR_ZERO:
                code_emit_zero(&codegen);
                break;
            default: basic_unreachable();
        }
    }

    code_eof(&codegen);
    da_free(&codegen.bracket_pos);
    return codegen;
}
