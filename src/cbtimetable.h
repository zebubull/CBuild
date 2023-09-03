#pragma once

#include <stdint.h>
#include <stdio.h>

#include "cbmem.h"
#include "cbstr.h"

typedef struct time_entry {
    cbstr_t file;
    cbstr_t parent;
    cbstr_t obj;
    int64_t write_time;
} time_entry_t;

typedef struct time_table {
    time_entry_t *entries;
    size_t len;
    size_t capacity;
} time_table_t;

#ifndef RELEASE

#define time_entry_free(entry) d_time_entry_free(entry, __FILE__, __LINE__)
#define time_table_init(cap) d_time_table_init(cap, __FILE__, __LINE__)
#define time_table_free(table) d_time_table_free(table, __FILE__, __LINE__)

#endif /* RELEASE */

void ALLOC_DEF(time_entry_free, time_entry_t *entry);

time_table_t ALLOC_DEF(time_table_init, size_t cap);
void ALLOC_DEF(time_table_free, time_table_t *table);
void time_table_push(time_table_t *table, time_entry_t entry);
void time_table_save(time_table_t *table, FILE *file);
void time_table_load(time_table_t *table, FILE *file);
time_entry_t *time_table_search(time_table_t *table, cbstr_t *file, cbstr_t *parent);