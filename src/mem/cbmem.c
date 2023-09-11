/// Author - zebubull
/// cbmem.c
/// cbmem.h implementation.
/// Copyright (c) zebubull 2023
#include "cbmem.h"

#include "../os/osdef.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct alloc {
    void *ptr;
    const char* file;
    size_t line;
    bool freed;
} alloc_t;

typedef struct alloc_list {
    alloc_t *allocs;
    size_t len;
    size_t capacity;
} alloc_list_t;

static alloc_list_t alloc_list;
static size_t total_allocs = 0;
static size_t expensive_reallocs = 0;

static alloc_list_t alloc_list_init(size_t capacity) {
    alloc_list_t list;
    list.allocs = (alloc_t*)CBMALLOC(capacity * sizeof(alloc_t));
    list.len = 0;
    list.capacity = capacity;
    return list;
}

static void alloc_list_push(alloc_list_t *list, alloc_t alloc) {
    if (list->len == list->capacity) {
        list->capacity = (list->capacity << 1) - (list->capacity >> 1);
        list->allocs = (alloc_t*)CBREALLOC(list->allocs, list->capacity * sizeof(alloc_t));
    }

    list->allocs[list->len] = alloc;
    ++list->len;
}

static alloc_t* alloc_list_search(alloc_list_t *list, void *ptr) {
    size_t i;

    for (i = 0; i < list->len; ++i) {
        if (list->allocs[i].ptr == ptr) {
            break;
        }
    }

    if (i == list->len) {
        return NULL;
    }

    return &list->allocs[i];
}

void debug_init() {
    alloc_list = alloc_list_init(16);
    printf("[DEBUG] cbmem debug init\n");
}

void debug_deinit() {
    size_t i;

    printf("[DEBUG] cbmem debug deinit\n");
    for (i = 0; i < alloc_list.len; ++i) {
        alloc_t *alloc = &alloc_list.allocs[i];
        if (!alloc->freed) {
            #ifdef _WIN32
            printf("[LEAK] %p leaked, alloc from %s:%I64d!\n", alloc->ptr, alloc->file, alloc->line);
            #endif
            #ifdef LINUX
            printf("[LEAK] %p leaked, alloc from %s:%lu!\n", alloc->ptr, alloc->file, alloc->line);
            #endif
        }
    }

    #ifdef _WIN32
    printf("[DEBUG] %I64d total allocs\n", total_allocs);
    printf("[DEBUG] %I64d expensive reallocs\n", expensive_reallocs);
    #endif
    #ifdef LINUX
    printf("[DEBUG] %lu total allocs\n", total_allocs);
    printf("[DEBUG] %lu expensive reallocs\n", expensive_reallocs);
    #endif
}

void *debug_alloc(size_t size, const char* file, size_t line) {
    alloc_t alloc;
    alloc_t *in_list;
    void *ptr;

    ptr = malloc(size);
    alloc.ptr = ptr;
    alloc.file = file;
    alloc.line = line;
    alloc.freed = false;

    in_list = alloc_list_search(&alloc_list, ptr);

    if (!in_list) {
        alloc_list_push(&alloc_list, alloc);
    } else {
        in_list->freed = false;
        in_list->line = line;
        in_list->file = file;
    }
    ++total_allocs;

    return ptr;
}

void debug_free(void *ptr, const char *file, size_t line) {
    alloc_t *alloc;
    alloc = alloc_list_search(&alloc_list, ptr);
    
    if (alloc->freed) {
        #ifdef _WIN32
        printf("[DFREE] %p double freed at %s:%I64d, alloc from %s:%I64d!\n", ptr, file, line, alloc->file, alloc->line);
        #endif
        #ifdef LINUX
        printf("[DFREE] %p double freed at %s:%lu, alloc from %s:%lu!\n", ptr, file, line, alloc->file, alloc->line);
        #endif
    } else {
        CBFREE(alloc->ptr);
        alloc->freed = true;
    }
}

void *debug_realloc(void *ptr, size_t size) {
    void *new_ptr;
    alloc_t *in_list;
    alloc_t alloc;

    new_ptr = CBREALLOC(ptr, size);

    if (new_ptr != ptr) {
        alloc_t *old_alloc;
        ++expensive_reallocs;
        old_alloc = alloc_list_search(&alloc_list, ptr);
        old_alloc->freed = true;

        alloc.ptr = new_ptr;
        alloc.file = old_alloc->file;
        alloc.line = old_alloc->line;
        alloc.freed = false;

        in_list = alloc_list_search(&alloc_list, new_ptr);

        if (!in_list) {
            alloc_list_push(&alloc_list, alloc);
        } else {
            in_list->freed = false;
            in_list->line = alloc.line;
            in_list->file = alloc.file;
        }
    }


    return new_ptr;
}
