#include <stdio.h>

#include "error.h"

typedef struct {
    size_t line;
    size_t col;
} Error_Loc;

void error_print(Error *error, const char *path, String_View source)
{
    Error_Loc loc = {
        .line = 1,
        .col = 1
    };

    for (size_t i = 0; i < error->pos; i++) {
        switch (source.data[i]) {
            case '\n':
                loc.line += 1;
                loc.col = 1;
                break;
            default:
                loc.col += 1;
                break;
        }
    }

    size_t start = error->pos;
    while (start > 0 && source.data[start-1] != '\n') {
        start -= 1;
    }

    size_t end = error->pos;
    while (end < source.count && source.data[end] != '\n') {
        end += 1;
    }

    int width = snprintf(NULL, 0, "%zu", loc.line);

    fprintf(stderr, "%s:%zu:%zu: %s\n", path, loc.line, loc.col, error->reason);
    fprintf(stderr, "%*s |\n", width, "");
    fprintf(stderr, "%zu | %.*s\n", loc.line, end - start, &source.data[start]);
    fprintf(stderr, "%*s | ", width, "");

    for (size_t i = start; i < error->pos; i++) {
        if (source.data[i] == '\t') {
            putc('\t', stderr);
        } else {
            putc(' ', stderr);
        }
    }
    fprintf(stderr, "^\n");
}

void errors_print(Errors *errors, const char *path)
{
    for (size_t i = 0; i < errors->count; i++) {
        error_print(&errors->items[i], path, errors->source);
        putc('\n', stderr);
    }
}
