/// Author - zebubull
/// cbmem.h
/// A header to make working with memory a bit easier when debugging.
/// Copyright (c) zebubull 2023
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef CBMALLOC
#define CBMALLOC(s) malloc(s)
#endif /* CBMALLOC */

#ifndef CBFREE
#define CBFREE(p) free(p)
#endif /* CBFREE */

#ifndef CBREALLOC
#define CBREALLOC(p, s) realloc(p, s)
#endif /* CBREALLOC */


#ifndef RELEASE

#define ALLOC_DEF(fn, ...) d_ ## fn(__VA_ARGS__, const char *file, size_t line)
#define MALLOC(s) debug_alloc(s, __FILE__, __LINE__)
#define FREE(p) debug_free(p, __FILE__, __LINE__)

#define WMALLOC(s, f, l) debug_alloc(s, f, l)
#define WFREE(p, f, l) debug_free(p, f, l)

#define FMALLOC(len) WMALLOC(len, file, line)
#define FFREE(ptr) WFREE(ptr, file, line)

#define FORWARD(call, ...) d_ ## call(__VA_ARGS__, file, line)

#define REALLOC(p, s) debug_realloc(p, s)

#define DEBUG_INIT() debug_init()
#define DEBUG_DEINIT() debug_deinit()

#else

#define ALLOC_DEF(fn, ...) d_ ## fn(__VA_ARGS__)
#define MALLOC(s) CBMALLOC(s)
#define FREE(p) CBFREE(p)

#define WMALLOC(s, f, l) MALLOC(s)
#define WFREE(p, f, l) FREE(s)

#define FMALLOC(len) MALLOC(len)
#define FFREE(ptr) FREE(ptr)

#define FORWARD(call, ...) d_ ## call(__VA_ARGS__)


#define REALLOC(p, s) CBREALLOC(p, s)

#define DEBUG_INIT()
#define DEBUG_DEINIT()

#endif /* RELEASE */

void debug_init();
void debug_deinit();
void *debug_alloc(size_t size, const char* file, size_t line);
void debug_free(void *ptr, const char *file, size_t line);
void *debug_realloc(void *ptr, size_t size);
