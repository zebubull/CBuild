/// Author - zebubull
/// dir.c
/// dir.h implementation
/// Copyright (c) zebubull 2023
#include "dir.h"
#include "osdef.h"

#ifdef _WIN32
#include <windows.h>
#include <ShlObj.h>

#define PATH_SEP '\\'
#define PATH_SEP_WIDE "\\"
#endif /* _WIN32 */

#ifdef UNIX
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define PATH_SEP '/'
#define PATH_SEP_WIDE "/"
#endif /* UNIX */
#include <stdbool.h>

#include "../mem/cbmem.h"
#include "../util/cbstr.h"

dir_t dir_init() {
    dir_t dir;
    dir.dir_names = cbstr_list_init(4);
    dir.entries = entry_list_init(4);
    return dir;
}

void dir_free(dir_t *dir) {
    cbstr_list_free(&dir->dir_names);
    entry_list_free(&dir->entries);
}

// TODO: make all of this code platform agnostic

#ifdef _WIN32

void walk_dir_windows(dir_t *dir, cbstr_t path) {
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
            walk_dir_windows(dir, new_path);
        } else {
            dir_entry_t entry;
            entry.parent = name_index;
            entry.filename = cbstr_from_cstr(find.cFileName, strnlen(find.cFileName, 260)+1);
            entry.write_time = (int64_t)(find.ftLastWriteTime.dwLowDateTime) | ((int64_t)(find.ftLastWriteTime.dwHighDateTime) << 32);
            entry_list_push(&dir->entries, entry);
        }
    } while (FindNextFileA(walk_handle, &find));

    FindClose(walk_handle);
    cbstr_free(&root);
}

#endif /* _WIN32 */

#ifdef UNIX

void walk_dir_linux(dir_t *dir, cbstr_t path) {
    cbstr_t root;

    size_t name_index;
    DIR *dir_handle;
    struct dirent *dirent;

    cbstr_list_push(&dir->dir_names, path);
    name_index = dir->dir_names.len - 1;
    root = cbstr_copy(&path);

    dir_handle = opendir(root.data);

    if (dir_handle == NULL) {
        cbstr_free(&root);
        return;
    }

    while ((dirent = readdir(dir_handle))) {
        struct stat statbuf;
        // Will go into dir name table and be freed later or get freed if not a dir
        cbstr_t full_path;

        full_path = cbstr_copy(cbstr_list_get(&dir->dir_names, name_index));
        cbstr_concat_cstr(&full_path, "/", 2);
        cbstr_concat_cstr(&full_path, dirent->d_name, strnlen(dirent->d_name, 256)+1);

        lstat(dirent->d_name, &statbuf);


        if (dirent->d_type == DT_REG) {
            dir_entry_t entry;
            entry.parent = name_index;
            entry.filename = cbstr_from_cstr(dirent->d_name, strnlen(dirent->d_name, 256)+1);
            #ifdef __linux__
            entry.write_time = statbuf.st_mtim.tv_sec;
            #endif /* __linux__ */
            #ifdef __APPLE__
            entry.write_time = statbuf.st_mtimespec.tv_sec;
            #endif /* __APPLE__ */
            entry_list_push(&dir->entries, entry);
            cbstr_free(&full_path);
        } else if (dirent->d_type == DT_DIR) {
            if (dirent->d_name[0] == '.' && (dirent->d_name[1] == 0 || (dirent->d_name[1] == '.' && dirent->d_name[2] == 0))) {
                cbstr_free(&full_path);
                continue;
            }
            walk_dir_linux(dir, full_path);
        }
    }

    closedir(dir_handle);
    cbstr_free(&root);
}

#endif /* UNIX */

dir_t walk_dir(cbstr_t path) {
    dir_t dir = dir_init();

    // Will go into dir name table and be freed later
    cbstr_t root = cbstr_copy(&path);

    #ifdef _WIN32
    walk_dir_windows(&dir, root);
    #endif

    #ifdef UNIX
    walk_dir_linux(&dir, root);
    #endif

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

void create_dir(char *path) {
    bool success;
    #ifdef _WIN32
    success = CreateDirectoryA(path, NULL) != 0;

    if (!success) {
        success = GetLastError() == ERROR_ALREADY_EXISTS;
    }
    #endif /* _WIN32 */

    #ifdef UNIX
    success = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
    if (!success) {
        success = errno != ENOENT;
    }
    #endif /* UNIX */

    if (!success) {
        size_t len;
        size_t i;

        len = strnlen(path, 260);
        for (i = len; i > 0; --i) {
            if (path[i] == PATH_SEP) {
                break;
            }
        }

        if (i == 0) {
            return;
        }

        path[i] = 0;
        create_dir(path);
        path[i] = PATH_SEP;

        #ifdef _WIN32
        CreateDirectoryA(path, NULL);
        #endif /* _WIN32 */

        #ifdef UNIX
        mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        #endif /* UNIX */
    }
}

bool file_exists(const char *path) {

    #ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES;
    #endif /* _WIN32 */

    #ifdef UNIX
    struct stat buffer;
    return (stat(path, &buffer) == 0);
    #endif /* UNIX */
}
