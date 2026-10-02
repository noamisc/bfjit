/*
  basic.h - (https://www.github.com/noadhum/basic.h)
  Personal C/C++ single-header library
*/

#ifndef BASIC_H_
#define BASIC_H_

#ifndef BASICDEF
#define BASICDEF
#endif // BASICDEF

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define basic_assert(expr, message) basic__assert(expr, __FILE__, __LINE__, __func__, message)
#define basic_assertf(expr, fmt, ...) basic__assertf(expr, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__)
#define basic_panic(message) basic__panic(__FILE__, __LINE__, __func__, message)
#define basic_panicf(fmt, ...) basic__panicf(__FILE__, __LINE__, __func__, fmt, __VA_ARGS__)

BASICDEF void basic__assert(bool expr, const char *file, size_t line, const char *func, const char *message);
BASICDEF void basic__assertf(bool expr, const char *file, size_t line, const char *func, const char *fmt, ...);
BASICDEF void basic__panic(const char *file, size_t line, const char *func, const char *message);
BASICDEF void basic__panicf(const char *file, size_t line, const char *func, const char *fmt, ...);

#define basic_return_defer(value) do {result = (value); goto defer;} while(0)
// tsoding/nob.h's nob_shift() and nob_shift_args()
#define basic_shift(xs, sz) (basic_assert((sz) > 0, "unable to shift anymore"), (sz)--, *(xs)++)
#define basic_shift_args(argc, argv) basic_shift(*argv, *argc)
#define basic_shift_many(xs, sz, count) (basic_assert((sz) - (count) > 0, "unable to shift anymore"), (sz) -= (count), (xs) += (count))

#define BASIC_ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))

#ifndef BASIC_DA_INIT_CAP
#define BASIC_DA_INIT_CAP 256
#endif // BASIC_DA_INIT_CAP

#ifdef __cplusplus
#define BASIC_DECLTYPE(T) (decltype(T))
#else
#define BASIC_DECLTYPE(T)
#endif

#ifdef __cplusplus
template <typename T>
struct Basic_Dynamic_Array {
    T *items;
    size_t count;
    size_t capacity;
};
#endif // __cplusplus

#define basic_da_reserve(da, new_cap)                                   \
    do {                                                                \
        if ((da)->capacity < (new_cap)) {                               \
            if ((da)->capacity == 0) (da)->capacity = BASIC_DA_INIT_CAP; \
            while ((da)->capacity < (new_cap)) {                        \
                (da)->capacity *= 2;                                    \
            }                                                           \
            (da)->items = BASIC_DECLTYPE((da)->items)realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            basic_assert((da)->items != NULL, "unable to allocate memory"); \
        }                                                               \
    } while(0)

#define basic_da_free(da) free((da)->items)

#define basic_da_append(da, item)                       \
    do {                                                \
        basic_da_reserve((da), (da)->count + 1);        \
        (da)->items[(da)->count++] = (item);            \
    } while(0)

#define basic_da_append_many(da, new_items, items_count)                \
    do {                                                                \
        basic_da_reserve((da), (da)->count + (items_count));            \
        memcpy((da)->items + (da)->count, (new_items), (items_count) * sizeof(*(da)->items)); \
        (da)->count += (items_count);                                   \
    } while(0)

#define basic_da_empty(da) ((da)->count == 0)

#define basic_da_push basic_da_append
#define basic_da_pop(da) (basic_assert((da)->count > 0, "unable to pop an empty dynamic array"), (da)->items[--(da)->count])
#define basic_da_peek_first(da) (basic_assert((da)->count > 0, "unable to peek first item in dynamic array"), (da)->items[0])
#define basic_da_peek_last(da) (basic_assert((da)->count > 0, "unable to peek last item in dynamic array"), (da)->items[(da)->count-1])

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} Basic_String_Builder;

#define basic_sb_reserve basic_da_reserve
#define basic_sb_free basic_da_free
#define basic_sb_append_sv(sb, sv) basic_da_append_many(sb, (sv)->data, (sv)->count)

BASICDEF void basic_sb_append_char(Basic_String_Builder *sb, char c);
BASICDEF void basic_sb_append_cstr(Basic_String_Builder *sb, const char *cstr);
BASICDEF void basic_sb_append_fmt(Basic_String_Builder *sb, const char *fmt, ...);

typedef struct {
    const char *data;
    size_t count;
} Basic_String_View;

#define BASIC_SV(cstr_lit) (Basic_String_View){(cstr_lit), sizeof(cstr_lit)-1}
#define basic_sv_shift(sv) ((sv)->count--, *(sv)->data++)
#define basic_sv_shift_many(sv, shift_count) ((sv)->count -= (shift_count), (sv)->data += (shift_count))

BASICDEF Basic_String_View basic_sv_from_cstr(const char *cstr);
BASICDEF Basic_String_View basic_sv_from_sb(const Basic_String_Builder *sb);
BASICDEF Basic_String_View basic_sv_substr(Basic_String_View sv, size_t pos, size_t count);
BASICDEF Basic_String_View basic_sv_slice(Basic_String_View sv, size_t start, size_t end);
BASICDEF bool basic_sv_eq_cstr(Basic_String_View sv, const char *cstr);
BASICDEF bool basic_sv_eq_sb(Basic_String_View sv, const Basic_String_Builder *sb);
BASICDEF bool basic_sv_eq_sv(Basic_String_View a, Basic_String_View b);
BASICDEF bool basic_sv_starts_with(Basic_String_View sv, const char *prefix);
BASICDEF bool basic_sv_ends_with(Basic_String_View sv, const char *suffix);

BASICDEF bool basic_read_file(const char *path, Basic_String_Builder *sb);

#ifndef BASIC_NO_PREFIX

#ifdef __cplusplus
#define Dynamic_Array Basic_Dynamic_Array
#endif // __cplusplus

#define da_reserve basic_da_reserve
#define da_free basic_da_free
#define da_append basic_da_append
#define da_append_many basic_da_append_many
#define da_empty basic_da_empty
#define da_push basic_da_push
#define da_pop basic_da_pop
#define da_peek_first basic_da_peek_first
#define da_peek_last basic_da_peek_last

#define String_Builder Basic_String_Builder

#define sb_reserve basic_sb_reserve
#define sb_free basic_sb_free
#define sb_append_sv basic_sb_append_sv
#define sb_append_char basic_sb_append_char
#define sb_append_cstr basic_sb_append_cstr
#define sb_append_fmt basic_sb_append_fmt

#define String_View Basic_String_View

#define SV BASIC_SV
#define sv_shift basic_sv_shift
#define sv_shift_many basic_sv_shift_many
#define sv_from_cstr basic_sv_from_cstr
#define sv_from_sb basic_sv_from_sb
#define sv_substr basic_sv_substr
#define sv_slice basic_sv_slice
#define sv_eq_cstr basic_sv_eq_cstr
#define sv_eq_sb basic_sv_eq_sb
#define sv_eq_sv basic_sv_eq_sv
#define sv_starts_with basic_sv_starts_with
#define sv_ends_with basic_sv_ends_with

#define read_file basic_read_file

#endif // BASIC_NO_PREFIX

#endif // BASIC_H_

#ifdef BASIC_IMPLEMENTATION

#ifndef BASIC_IMPLEMENTED
#define BASIC_IMPLEMENTED

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

BASICDEF void basic__panic(const char *file, size_t line, const char *func, const char *message)
{
    fprintf(stderr, "%s:%zu: %s: %s\n", file, line, func, message);
    abort();
}

BASICDEF void basic__panicf(const char *file, size_t line, const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%zu: %s: ", file, line, func);
    vfprintf(stderr, fmt, ap);
    putc('\n', stderr);
    va_end(ap);
    abort();
}

BASICDEF void basic__assert(bool expr, const char *file, size_t line, const char *func, const char *message)
{
    if (!expr) {
        basic__panic(file, line, func, message);
    }
}

BASICDEF void basic__assertf(bool expr, const char *file, size_t line, const char *func, const char *fmt, ...)
{
    if (!expr) {
        va_list ap;
        va_start(ap, fmt);

        fprintf(stderr, "%s:%zu: %s: ", file, line, func);
        vfprintf(stderr, fmt, ap);
        putc('\n', stderr);

        va_end(ap);
        abort();
    }
}

BASICDEF void basic_sb_append_char(Basic_String_Builder *sb, char c)
{
    basic_da_append(sb, c);
}

BASICDEF void basic_sb_append_cstr(Basic_String_Builder *sb, const char *cstr)
{
    size_t cstr_count = strlen(cstr);
    basic_da_append_many(sb, cstr, cstr_count);
}

BASICDEF void basic_sb_append_fmt(Basic_String_Builder *sb, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int fmt_count = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    va_start(ap, fmt);
    basic_da_reserve(sb, sb->count + fmt_count + 1);
    vsnprintf(sb->items + sb->count, fmt_count + 1, fmt, ap);
    va_end(ap);

    sb->count += (size_t)fmt_count;
}

BASICDEF Basic_String_View basic_sv_from_cstr(const char *cstr)
{
    return (Basic_String_View){cstr, strlen(cstr)};
}

BASICDEF Basic_String_View basic_sv_from_sb(const Basic_String_Builder *sb)
{
    return (Basic_String_View){sb->items, sb->count};
}

BASICDEF Basic_String_View basic_sv_substr(Basic_String_View sv, size_t pos, size_t count)
{
    return (Basic_String_View){sv.data + pos, count};
}

BASICDEF Basic_String_View basic_sv_slice(Basic_String_View sv, size_t start, size_t end)
{
    if (start > end) {
        return (Basic_String_View){sv.data, 0};
    } else {
        return (Basic_String_View){sv.data + start, end - start};
    }
}

BASICDEF bool basic_sv_eq_cstr(Basic_String_View sv, const char *cstr)
{
    size_t cstr_count = strlen(cstr);
    if (sv.count != cstr_count) return false;
    return memcmp(sv.data, cstr, sv.count) == 0;
}

BASICDEF bool basic_sv_eq_sb(Basic_String_View sv, const Basic_String_Builder *sb)
{
    if (sv.count != sb->count) return false;
    return memcmp(sv.data, sb->items, sv.count) == 0;
}

BASICDEF bool basic_sv_eq_sv(Basic_String_View a, Basic_String_View b)
{
    if (a.count != b.count) return false;
    return memcmp(a.data, b.data, a.count) == 0;
}

BASICDEF bool basic_sv_starts_with(Basic_String_View sv, const char *prefix)
{
    size_t prefix_count = strlen(prefix);

    if (sv.count < prefix_count) return false;

    Basic_String_View expected_prefix = {prefix, prefix_count};
    Basic_String_View actual_prefix = basic_sv_substr(sv, 0, prefix_count);

    return basic_sv_eq_sv(expected_prefix, actual_prefix);
}

BASICDEF bool basic_sv_ends_with(Basic_String_View sv, const char *suffix)
{
    size_t suffix_count = strlen(suffix);

    if (sv.count < suffix_count) return false;

    Basic_String_View expected_suffix = {suffix, suffix_count};
    Basic_String_View actual_suffix = basic_sv_substr(sv, sv.count - expected_suffix.count, expected_suffix.count);

    return basic_sv_eq_sv(expected_suffix, actual_suffix);
}

BASICDEF bool basic_read_file(const char *path, Basic_String_Builder *sb)
{
     bool result = true;
     long long count;

     FILE *fp = fopen(path, "rb");
     if (!fp) basic_return_defer(false);
     if (fseek(fp, 0, SEEK_END) != 0) basic_return_defer(false);

#ifdef _WIN32
     count = _ftelli64(fp);
#else
     count = ftello(fp);
#endif

     if (count == -1L) basic_return_defer(false);
     if (fseek(fp, 0, SEEK_SET) != 0) basic_return_defer(false);

     basic_da_reserve(sb, count);
     fread(sb->items + sb->count, count, 1, fp);
     if (ferror(fp)) basic_return_defer(false);
     sb->count += count;

defer:
     if (fp) fclose(fp);
     return result;
}

#endif // BASIC_IMPLEMENTED

#endif // BASIC_IMPLEMENTATION
