#pragma once

#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef RELEASE
#define MALLOC(s) debug_alloc(s, __FILE__, __LINE__)
#define FREE(p) debug_free(p, __FILE__, __LINE__)

#define MALLOCW(s, f, l) debug_alloc(s, f, l)
#define FREEW(p, f, l) debug_free(p, f, l)

#define REALLOC(p, s) debug_realloc(p, s)

#define DEBUG_INIT() debug_init()
#define DEBUG_DEINIT() debug_deinit()
#else
#define MALLOC(s) malloc(s)
#define FREE(p) free(p)
#define REALLOC(p, s) realloc(p, s)
#define DEBUG_INIT()
#define DEBUG_DEINIT()
#endif /* RELEASE */

void debug_init();
void debug_deinit();
void *debug_alloc(size_t size, const char* file, size_t line);
void debug_free(void *ptr, const char *file, size_t line);
void *debug_realloc(void *ptr, size_t size);
