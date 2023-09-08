/// Author - zebubull
/// dir.h
/// A header for working with directories.
/// Copyright (c) zebubull 2023
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "../util/cbstr.h"

typedef struct dir_entry {
    size_t parent;
    cbstr_t filename;
    time_t write_time;
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
void dir_free(dir_t *dir);

entry_list_t entry_list_init(size_t cap);
void entry_list_push(entry_list_t *list, dir_entry_t entry);
dir_entry_t* entry_list_get(entry_list_t *list, size_t item);
void entry_list_free(entry_list_t *list);

void create_dir(char *path);
bool file_exists(const char *path);

// TODO: add api for creating directories and checking if files exist
