#pragma once
#include <stdbool.h>
#include "cbstr.h"

#ifndef _WIN32
#error "The dir module only supports windows"
#endif

typedef struct dir_entry {
    size_t parent;
    cbstr_t filename;
} dir_entry_t;

typedef struct entry_list {
    dir_entry_t *entries;
    size_t len;
    size_t cap;
} entry_list_t;

typedef struct dir {
    entry_list_t entries;
    cbstr_list_t dir_names;
} dir_t;

dir_t walk_dir(cbstr_t path);
void free_dir(dir_t *dir);

entry_list_t entry_list_init(size_t cap);
void entry_list_push(entry_list_t *list, dir_entry_t entry);
dir_entry_t* entry_list_get(entry_list_t *list, size_t item);
void entry_list_free(entry_list_t *list);