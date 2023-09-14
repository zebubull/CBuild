/// Author - zebubull
/// cbtimetable.c
/// cbtimetable.h implementation.
/// Copyright (c) zebubull 2023
#include "cbtimetable.h"

#include "cbstr.h"
#include "../os/osdef.h"
#include "../mem/cbmem.h"
#include "../util/cblog.h"

#include <stdio.h>

void ALLOC_DEF(tt_entry_free, tt_entry_t *entry) {
    FORWARD(cbstr_free, &entry->file_name);
    FORWARD(cbstr_free, &entry->parent_dirs);
    FORWARD(cbstr_free, &entry->obj_file);
}

tt_t ALLOC_DEF(tt_init, size_t cap) {
    tt_t table;
    table.len = 0;
    table.capacity = cap;
    table.files = FMALLOC(cap * sizeof(tt_entry_t));

    return table;
}

void ALLOC_DEF(tt_free, tt_t *table) {
    size_t i;

    for (i = 0; i < table->len; ++i) {
        FORWARD(tt_entry_free, table->files + i);
    }

    FFREE(table->files);
}

void tt_push(tt_t *table, tt_entry_t entry) {
    if (table->len == table->capacity) {
        // capacity *= 1.5
        table->capacity = (table->capacity << 1) - (table->capacity >> 1);
        table->files = REALLOC(table->files, table->capacity * sizeof(tt_entry_t));
    }

    table->files[table->len] = entry;
    ++table->len;
}

void write_cbstr(cbstr_t *str, FILE *file) {
    const uint32_t len = str->len;
    fwrite(&len, sizeof(len), 1, file);
    fwrite(str->data, 1, len, file);
}

void tt_save(tt_t *table, FILE *file) {
    size_t i;
    const uint16_t magic_num = TT_MAGIC;
    const uint16_t version = TT_VERSION;
    const uint32_t num_entries = (uint32_t)table->len;

    fwrite(&magic_num, 1, sizeof(magic_num), file);
    fwrite(&version, 1, sizeof(version), file);
    fwrite(&num_entries, 1, sizeof(num_entries), file);
    fwrite(&table->build_success, 1, sizeof(table->build_success), file);

    for (i = 0; i < num_entries; ++i) {
        tt_entry_t *entry = table->files + i;

        fwrite(&entry->write_time, sizeof(entry->write_time), 1, file);
        write_cbstr(&entry->file_name, file);
        write_cbstr(&entry->parent_dirs, file);
        write_cbstr(&entry->obj_file, file);
    }

    fflush(file);
}

cbstr_t read_cbstr(FILE *file) {
    uint32_t len;
    cbstr_t str;

    fread(&len, 1, sizeof(len), file);
    str = cbstr_with_cap(len);
    fread(str.data, 1, len, file);
    str.len = len;

    return str;
}

void tt_load(tt_t *table, FILE *file) {
    size_t i;
    uint16_t magic_num;
    uint16_t version;
    uint32_t num_entries;

    fread(&magic_num, 1, sizeof(magic_num), file);

    if (magic_num != TT_MAGIC) {
        *table = tt_init(4);
        eprintf("[WARNING] Invalid timetable file, skipping incremental compilation...\n");
        return;
    }

    fread(&version, 1, sizeof(version), file);

    if (version != TT_VERSION) {
        *table = tt_init(4);
        eprintf("[WARNING] Outdated timetable file, skipping incremental compilation...\n");
        return;
    }

    fread(&num_entries, 1, sizeof(num_entries), file);

    *table = tt_init(num_entries);

    fread(&table->build_success, 1, sizeof(table->build_success), file);

    for (i = 0; i < num_entries; ++i) {
        tt_entry_t entry;

        fread(&entry.write_time, 1, sizeof(entry.write_time), file);
        entry.file_name = read_cbstr(file);
        entry.parent_dirs = read_cbstr(file);
        entry.obj_file = read_cbstr(file);

        tt_push(table, entry);
    }
}

tt_entry_t *tt_search(tt_t *table, cbstr_t *file, cbstr_t *parent) {
    size_t i;

    for (i = 0; i < table->len; ++i) {
        tt_entry_t *entry = &table->files[i];
        if (cbstr_cmp(&entry->file_name, file) && cbstr_cmp(&entry->parent_dirs, parent)) {
            return entry;
        }
    }

    return NULL;
}
