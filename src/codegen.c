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
    }
}

static void code_deinit(Codegen *codegen)
{
    da_free(&codegen->bracket_pos);
}

static void code_eof(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // push rbx
            code_emit_u8(&codegen->code, '\x5B');
            // ret
            code_emit_u8(&codegen->code, '\xC3');
        } break;
    }
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
                    } break;
                    case OS_MACOS:
                    case OS_LINUX: {
                        // movzx rdi, byte[rbx]
                        code_emit_str(&codegen->code, "\x48\x0F\xB6\x3B");
                    } break;
                }
                // call 0x00000000
                code_emit_u8(&codegen->code, '\xE8');
                da_append(&codegen->call_stack, ((Call){CALL_PUT, codegen->code.count}));
                code_emit_u32(&codegen->code, 0);
            } break;
        }
    }
}

static void code_emit_get(Codegen *codegen, size_t operand)
{
    for (size_t i = 0; i < operand; i++) {
        switch (get_target_arch()) {
            case ARCH_X64: {
                // call 0x00000000
                code_emit_u8(&codegen->code, '\xE8');
                da_append(&codegen->call_stack, ((Call){CALL_GET, codegen->code.count}));
                code_emit_u32(&codegen->code, 0);
                // mov [rbx], al
                code_emit_str(&codegen->code, "\x88\x03");
            } break;
        }
    }
}

static void code_emit_jz(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // cmp byte[rbx], 0
            code_emit_str(&codegen->code, "\x80\x3B\x00");
            // jz 0x00000000
            code_emit_str(&codegen->code, "\x0F\x84");
            da_append(&codegen->bracket_pos, codegen->code.count);
            code_emit_u32(&codegen->code, 0);
        } break;
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

            size_t jz_addr = da_pop(&codegen->bracket_pos);
            int32_t jz_operand = (int32_t)(codegen->code.count - jz_addr);
            memcpy(codegen->code.items + jz_addr, &jz_operand, sizeof(jz_operand));

            int32_t jnz_operand = (int32_t)jz_addr - (int32_t)codegen->code.count;
            code_emit_u32(&codegen->code, (uint32_t)jnz_operand);
        } break;
    }
}

static void code_emit_zero(Codegen *codegen)
{
    switch (get_target_arch()) {
        case ARCH_X64: {
            // mov byte[rbx], 0
            code_emit_str(&codegen->code, "\xC6\x03\x00");
        } break;
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
        }
    }

    code_eof(&codegen);
    return codegen;
}
