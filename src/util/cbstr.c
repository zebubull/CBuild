/// Author - zebubull
/// cbstr.c
/// cbstr.h implementation.
/// Copyright (c) zebubull 2023
#include "cbstr.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "../mem/cbmem.h"


cbstr_t ALLOC_DEF(cbstr_from_cstr, const char *cstr, size_t len) {
    cbstr_t str;

    if (cstr[len-1] != '\0') {
        ++len;
        str.data = FMALLOC(len);
        memcpy(str.data, cstr, len-1);
        str.data[len-1] = 0;
    } else {
        str.data = FMALLOC(len);
        memcpy(str.data, cstr, len);
    }

    str.len = len;
    str.capacity = len;

    return str;
}

cbstr_t ALLOC_DEF(cbstr_copy, cbstr_t *str) {
    cbstr_t new_str;
    new_str.len = str->len;
    new_str.capacity = str->len;

    new_str.data = FMALLOC(new_str.capacity);
    memcpy(new_str.data, str->data, new_str.capacity);

    return new_str;
}

cbstr_t ALLOC_DEF(cbstr_with_cap, size_t cap) {
    cbstr_t str;
    str.len = cap;
    str.capacity = cap;
    str.data = FMALLOC(cap);
    cbstr_clear(&str);

    return str;
}

void cbstr_clear(cbstr_t* str) {
    memset(str->data, 0, str->len);
    str->len = 0;
}

void ALLOC_DEF(cbstr_free, cbstr_t *str) {
    FFREE(str->data);
    str->capacity = 0;
    str->len = 0;
}

void cbstr_concat(cbstr_t *a, cbstr_t *b) {
    cbstr_concat_cstr(a, b->data, b->len);
}

void cbstr_concat_slice(cbstr_t *a, cbstr_t *b, size_t offset) {
    if (offset >= b->len) return;
    b->data += offset;
    b->len -= offset;
    cbstr_concat(a, b);
    b->data -= offset;
    b->len += offset;
}

void cbstr_concat_format(cbstr_t *a, const char *format, size_t len, ...) {
    size_t start;
    size_t i;
    size_t in_format;
    va_list args;

    va_start(args, len);
    start = 0;
    in_format = false;
    for (i = 0; i < len; ++i) {
        switch (format[i]) {
            case '%':
            {
                if (in_format) {
                    in_format = 0;
                    cbstr_concat_cstr(a, "%", sizeof("%"));
                    start = i+1;
                } else {
                    in_format = 1;
                    cbstr_concat_cstr(a, format+start, i-start);
                }
            }
            break;
            case 's':
            {
                if (in_format) {
                    cbstr_t* b;
                    in_format = 0;
                    start = i+1;
                    b = va_arg(args, cbstr_t*);
                    cbstr_concat(a, b);
                }
            }
            break;
            default:
            break;
        }

        if (in_format == 2) {
            in_format = 0;
        }

        if (in_format == 1) {
            ++in_format;
        }
    }

    cbstr_concat_cstr(a, format+start, i-start);
    va_end(args);
}

void cbstr_concat_cstr(cbstr_t *a, const char *b, size_t len) {
    // Null terminator of first string is discarded
    size_t target;
    size_t new_len;
    bool needs_zero = false;
    if (len == 0) return;

    new_len = a->len + len - 1;

    if (b[len-1] != 0) {
        needs_zero = true;
        ++new_len;
    }

    if (a->len == 0) {
        target = 0;
        ++new_len;
    } else {
        target = a->len - 1;
    }

    if (a->capacity < new_len) {
        size_t new_cap = (new_len << 1) - (new_len >> 1);
        a->data = REALLOC(a->data, new_cap);
        a->capacity = new_cap;
    }
    
    memcpy(a->data + target, b, len);
    a->len = new_len;

    if (needs_zero) {
        a->data[new_len] = 0;
    }
}

bool cbstr_cmp(cbstr_t *a, cbstr_t *b) {
    size_t i;

    if (a->len != b-> len) {
        return false;
    }

    for (i = 0; i < a->len; ++i) {
        if (a->data[i] != b->data[i]) {
            return false;
        }
    }

    return true;
}

cbstr_list_t ALLOC_DEF(cbstr_list_init, size_t cap) {
    cbstr_list_t list = {.cap = cap, .len = 0, .strings = FMALLOC(cap * sizeof(cbstr_t))};
    return list;
}

void ALLOC_DEF(cbstr_list_free, cbstr_list_t *list) {
    for (size_t i = 0; i < list->len; ++i) {
        #ifndef RELEASE
        d_cbstr_free(&list->strings[i], file, l);
        #else
        cbstr_free(&list->strings[i]);
        #endif
    }
    FFREE(list->strings);
}

void cbstr_list_push(cbstr_list_t *list, cbstr_t str) {
    if (list->len == list->cap) {
        list->cap = (list->cap << 2) - (list->cap >> 1);
        list->strings = REALLOC(list->strings, list->cap * sizeof(cbstr_t));
    }

    list->strings[list->len] = str;
    ++list->len;
}

cbstr_t* cbstr_list_get(cbstr_list_t *list, size_t item) {
    if (item > list->len) return NULL;
    return &list->strings[item];
}
