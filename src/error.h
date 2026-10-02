#ifndef ERROR_H_
#define ERROR_H_

#include "basic.h"

typedef struct {
    const char *reason;
    size_t pos;
} Error;

typedef struct {
    String_View source;
    Error *items;
    size_t count;
    size_t capacity;
} Errors;

void error_print(Error *error, const char *path, String_View source);
void errors_print(Errors *errors, const char *path);

#endif // ERROR_H_
