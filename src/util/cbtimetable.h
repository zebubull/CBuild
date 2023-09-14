/// Author - zebubull
/// cbtimetable.h
/// A header for keeping track of file write times.
/// Copyright (c) zebubull 2023

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "../mem/cbmem.h"
#include "../util/cbstr.h"
#include "../os/time.h"

#define TT_VERSION 3
#define TT_MAGIC 0x5474

// Timetable file structure
// +----------------------+---------+
// | Magic Number - 54 74 | 2 Bytes |
// +----------------------+---------+
// | File Version         | 2 Bytes |
// +----------------------+---------+
// | Number of entries    | 4 Bytes |
// +----------------------+---------+
// | Build success status | 1 Byte  |
// +----------------------+---------+
// | Entries              | Varies  |
// +----------------------+---------+
//
// Timetable entry structure
// Note - all strings are length-prefixed (4 bytes) and null-terminated
// +----------------------+---------+
// | Last write time      | 8 Bytes |
// +----------------------+---------+
// | File name            | String  |
// +----------------------+---------+
// | Parent directories   | String  |
// +----------------------+---------+
// | Object file          | String  |
// +----------------------+---------+

typedef struct tt_header {
    uint16_t magic;
    uint16_t version;
    uint32_t size;
} tt_header_t;

typedef struct tt_entry {
    time_t write_time;

    // Store name separately from directory to (maybe) speed up search (remind me to benchmark later)
    cbstr_t file_name;
    cbstr_t parent_dirs;

    // This probably doesn't need to be stored but I will keep it in for now
    cbstr_t obj_file;
} tt_entry_t;

typedef struct tt {
    tt_entry_t *files;
    size_t len;
    size_t capacity;
    bool build_success;
} tt_t;

#ifdef DEBUG
#define tt_entry_free(entry) d_tt_entry_free(entry, __FILE__, __LINE__)
#define tt_init(cap) d_tt_init(cap, __FILE__, __LINE__)
#define tt_free(table) d_tt_free(table, __FILE__, __LINE__)
#endif /* DEBUG */

#ifdef RELEASE
#define tt_entry_free(entry) d_tt_entry_free(entry)
#define tt_init(cap) d_tt_init(cap)
#define tt_free(table) d_tt_free(table)
#endif /* RELEASE */

void ALLOC_DEF(tt_entry_free, tt_entry_t *entry);
tt_t ALLOC_DEF(tt_init, size_t cap);
void ALLOC_DEF(tt_free, tt_t *table);

void tt_push(tt_t *table, tt_entry_t entry);
void tt_save(tt_t *table, FILE *file);
void tt_load(tt_t *table, FILE *file);
tt_entry_t *tt_search(tt_t *table, cbstr_t *file, cbstr_t *parent);
