#include <time.h>

#define BASIC_IMPLEMENTATION
#include "basic.h"

#include "bf_ir.h"
#include "bf_vm.h"
#include "error.h"

typedef enum {
    CMD_NONE,
    CMD_HELP,
    CMD_INTERPRET,
    CMD_JIT_COMPILE,
} Command;

static void bfjit_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
}

static void bfjit_error_then_exit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
    exit(1);
}

static void print_usage(const char *exec)
{
    printf("usage: %s [command] [option]\n"
           "\n"
           "commands:\n"
           "  help             print this help then exit\n"
           "  interpret        execute source file with the interpreter\n"
           "  jit-compile      execute source file with just-in-time compilation\n", exec);
}

int main(int argc, char **argv)
{
    int result = 0;

    const char *exec = argv[0];
    Command command = CMD_NONE;
    const char *path = NULL;
    bool info = false;

    if (argc >= 2) {
        if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            command = CMD_HELP;
        } else if (strcmp(argv[1], "interpret") == 0) {
            command = CMD_INTERPRET;
        } else if (strcmp(argv[1], "jit-compile") == 0) {
            command = CMD_JIT_COMPILE;
        } else {
            bfjit_error_then_exit("error: unrecognized command '%s'", argv[1]);
        }
    }

    if (command == CMD_NONE || command == CMD_HELP) {
        print_usage(exec);
        return 0;
    }

    if (command == CMD_INTERPRET || command == CMD_JIT_COMPILE) {
        for (size_t i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (strcmp(argv[1], "--info") == 0) {
                    info = true;
                }
            } else {
                if (!path) {
                    path = argv[i];
                } else {
                    bfjit_error_then_exit("error: unexpected input file(s)");
                }
            }
        }
    }

    if (!path) {
        bfjit_error_then_exit("error: expected input file");
    }

    String_Builder source = {0};
    Bf_IRs bf_irs = {0};
    Bf_IRs optimized_irs = {0};
    Errors errors = {0};

    Bf_VM bf_vm = {0};

    if (!basic_read_file(path, &source)) {
        bfjit_error("error: unable to read file '%s'", path);
        basic_return_defer(1);
    }

    bf_irs = generate_bf_irs(sv_from_sb(&source));
    if (!parse_bf_irs(&bf_irs, &errors)) {
        errors_print(&errors, path);
        bfjit_error("error: bfjit terminated due to previous syntax error(s)");
        basic_return_defer(1);
    }

    optimized_irs = optimize_bf_irs(&bf_irs);
    bf_vm = bf_vm_init(&optimized_irs);

    switch (command) {
        case CMD_INTERPRET:
            bf_vm_interpret(&bf_vm);
            break;
        case CMD_JIT_COMPILE:
            bf_vm_jit_compile(&bf_vm);
            break;
    }

defer:
    if (source.items) sb_free(&source);
    if (bf_irs.items) da_free(&bf_irs);
    if (optimized_irs.items) da_free(&optimized_irs);
    if (errors.items) da_free(&errors);
    bf_vm_deinit(&bf_vm);
    return result;
}
