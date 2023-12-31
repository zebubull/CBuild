/// Author - zebubull
/// cbstr.h
/// A header for memory-safe dynamic string operations.
/// Copyright (c) zebubull 2023
#pragma once

#include "../mem/cbmem.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct cbstr {
    char *data;
    size_t len;
    size_t capacity;
} cbstr_t;

typedef struct cbstr_list {
    cbstr_t *strings;
    size_t len;
    size_t cap;
} cbstr_list_t;

#ifdef DEBUG
#define cbstr_from_cstr(cstr, len) d_cbstr_from_cstr(cstr, len, __FILE__, __LINE__)
#define cbstr_from_lit(cstr) d_cbstr_from_cstr(cstr, sizeof(cstr), __FILE__, __LINE__)
#define cbstr_with_cap(cap) d_cbstr_with_cap(cap, __FILE__, __LINE__)
#define cbstr_copy(str) d_cbstr_copy(str, __FILE__, __LINE__)
#define cbstr_free(str) d_cbstr_free(str, __FILE__, __LINE__)

#define cbstr_list_init(cap) d_cbstr_list_init(cap, __FILE__, __LINE__);
#define cbstr_list_free(list) d_cbstr_list_free(list, __FILE__, __LINE__)
#endif /* DEBUG */

#ifdef RELEASE
#define cbstr_from_cstr(cstr, len) d_cbstr_from_cstr(cstr, len)
#define cbstr_from_lit(cstr) d_cbstr_from_cstr(cstr, sizeof(cstr))
#define cbstr_with_cap(cap) d_cbstr_with_cap(cap)
#define cbstr_copy(str) d_cbstr_copy(str)
#define cbstr_free(str) d_cbstr_free(str)

#define cbstr_list_init(cap) d_cbstr_list_init(cap)
#define cbstr_list_free(list) d_cbstr_list_free(list)
#endif /* RELEASE */

#define CB_CSTR(s) s, sizeof(s)

cbstr_t ALLOC_DEF(cbstr_from_cstr, const char* cstr, size_t len);
cbstr_t ALLOC_DEF(cbstr_with_cap, size_t cap);
cbstr_t ALLOC_DEF(cbstr_copy, cbstr_t *str);
void ALLOC_DEF(cbstr_free, cbstr_t *str);

void cbstr_concat(cbstr_t *a, cbstr_t *b);
void cbstr_concat_slice(cbstr_t *a, cbstr_t *b, size_t offset);
void cbstr_concat_format(cbstr_t *a, const char *format, size_t len, ...);
void cbstr_concat_cstr(cbstr_t *a, const char *b, size_t len);
void cbstr_clear(cbstr_t* str);
void cbstr_localize_path(cbstr_t *str);
bool cbstr_cmp(cbstr_t *a, cbstr_t *b);

cbstr_list_t ALLOC_DEF(cbstr_list_init, size_t cap);
void ALLOC_DEF(cbstr_list_free, cbstr_list_t *list);
void cbstr_list_push(cbstr_list_t *list, cbstr_t str);
cbstr_t* cbstr_list_get(cbstr_list_t *list, size_t item);
