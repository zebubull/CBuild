#include "dir.h"

#include <windows.h>
#include <stdbool.h>

#include "cbmem.h"

dir_t dir_init() {
    dir_t dir;
    dir.dir_names = cbstr_list_init(4);
    dir.entries = entry_list_init(4);
    return dir;
}

void free_dir(dir_t *dir) {
    cbstr_list_free(&dir->dir_names);
    entry_list_free(&dir->entries);
}

void walk_dir_recursive(dir_t *dir, cbstr_t path) {
    cbstr_t root;

    size_t name_index;
    HANDLE walk_handle;
    WIN32_FIND_DATAA find;

    cbstr_list_push(&dir->dir_names, path);
    name_index = dir->dir_names.len - 1;
    root = cbstr_copy(&path);
    cbstr_concat_cstr(&root, "\\*", 3);

    walk_handle = FindFirstFileA(root.data, &find);

    if (walk_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            cbstr_t new_path;
            if (find.cFileName[0] == '.' && (find.cFileName[1] == 0 || (find.cFileName[1] == '.' && find.cFileName[2] == 0))) {
                continue;
            }
            new_path = cbstr_copy(cbstr_list_get(&dir->dir_names, name_index));
            cbstr_concat_cstr(&new_path, "\\", 2);
            cbstr_concat_cstr(&new_path, find.cFileName, strnlen(find.cFileName, 260)+1);
            walk_dir_recursive(dir, new_path);
        } else {
            dir_entry_t entry;
            entry.parent = name_index;
            entry.filename = cbstr_from_cstr(find.cFileName, strnlen(find.cFileName, 260)+1);
            entry.write_time = (int64_t)(find.ftLastWriteTime.dwLowDateTime) | ((int64_t)(find.ftLastWriteTime.dwHighDateTime) << 32);
            entry_list_push(&dir->entries, entry);
        }
    } while (FindNextFileA(walk_handle, &find));

    cbstr_free(&root);
}

dir_t walk_dir(cbstr_t path) {
    dir_t dir = dir_init();

    cbstr_t root = cbstr_copy(&path);
    walk_dir_recursive(&dir, root);

    return dir;
}

entry_list_t entry_list_init(size_t cap) {
    entry_list_t list = {.cap = cap, .len = 0, .entries = MALLOC(cap * sizeof(dir_entry_t))};
    return list;
}

void entry_list_push(entry_list_t *list, dir_entry_t entry) {
    if (list->len == list->cap) {
        list->cap = (list->cap << 1) - (list->cap >> 1);
        list->entries = REALLOC(list->entries, list->cap * sizeof(dir_entry_t));
    }

    list->entries[list->len] = entry;
    ++list->len;
}

dir_entry_t* entry_list_get(entry_list_t *list, size_t item) {
    if (item > list->len) return NULL;
    return &list->entries[item];
}

void entry_list_free(entry_list_t *list) {
    for (size_t i = 0; i < list->len; ++i) {
        cbstr_free(&list->entries[i].filename);
    }
    FREE(list->entries);
}