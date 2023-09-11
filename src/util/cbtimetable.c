/// Author - zebubull
/// cbtimetable.c
/// cbtimetable.h implementation.
/// Copyright (c) zebubull 2023
#include "cbtimetable.h"

#include "cbstr.h"
#include "../os/osdef.h"
#include "../mem/cbmem.h"

#include <stdio.h>

void ALLOC_DEF(time_entry_free, time_entry_t *entry) {
    cbstr_free(&entry->file);
    cbstr_free(&entry->parent);
    cbstr_free(&entry->obj);
}

time_table_t ALLOC_DEF(time_table_init, size_t cap) {
    time_table_t table;
    table.len = 0;
    table.capacity = cap;
    table.entries = FMALLOC(cap * sizeof(time_entry_t));

    return table;
}

void ALLOC_DEF(time_table_free, time_table_t *table) {
    size_t i;

    for (i = 0; i < table->len; ++i) {
        #ifndef RELEASE
        d_time_entry_free(&table->entries[i], file, l);
        #else 
        time_entry_free(&table->entries[i]);
        #endif
    }

    FFREE(table->entries);
}

void time_table_push(time_table_t *table, time_entry_t entry) {
    if (table->len == table->capacity) {
        table->capacity = (table->capacity << 2) - (table->capacity >> 1);
        table->entries = REALLOC(table->entries, table->capacity * sizeof(time_entry_t));
    }

    table->entries[table->len] = entry;
    ++table->len;
}

void time_table_save(time_table_t *table, FILE *file) {
    #ifdef _WIN32 
    #define INT_FORMAT "%I64d "
    #define FILE_FORMAT "%s %s %s %I64d "
    #endif
    #ifdef LINUX
    #define INT_FORMAT "%lu "
    #define FILE_FORMAT "%s %s %s %lu "
    #endif

    size_t i;
    fprintf(file, INT_FORMAT, table->len);

    for (i = 0; i < table->len; ++i) {
        time_entry_t *entry = &table->entries[i];
        fprintf(file, FILE_FORMAT, entry->file.data, entry->parent.data, entry->obj.data, entry->write_time);
    }
}

void time_table_load(time_table_t *table, FILE *file) {
    #ifdef _WIN32 
    #define FILE_FORMAT "%260s %260s %260s %I64d "
    #endif
    #ifdef LINUX
    #define FILE_FORMAT "%260s %260s %260s %lu "
    #endif

    size_t len;
    size_t i;
    fscanf(file, INT_FORMAT, &len);
    
    for (i = 0; i < len; ++i) {
        // I don't think this can segfault anymore but I don't trust myself
        char name[260];
        char parent[260];
        char obj[260];
        time_entry_t entry;

        if (fscanf(file, FILE_FORMAT, name, parent, obj, &entry.write_time) != 4) {
            printf("[WARNING] Failed to load time table...\n");
            break;
        }

        entry.file = cbstr_from_cstr(name, strnlen(name, 260));
        entry.parent = cbstr_from_cstr(parent, strnlen(parent, 260));
        entry.obj = cbstr_from_cstr(obj, strnlen(obj, 260));

        time_table_push(table, entry);
    }
    #undef FILE_FORMAT
    #undef INT_FORMAT
}

time_entry_t *time_table_search(time_table_t *table, cbstr_t *file, cbstr_t *parent) {
    size_t i;

    for (i = 0; i < table->len; ++i) {
        time_entry_t *entry = &table->entries[i];
        if (cbstr_cmp(&entry->file, file) && cbstr_cmp(&entry->parent, parent)) {
            return entry;
        }
    }

    return NULL;
}
