#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>

#include "dir.h"
#include "cbmem.h"

#define FLEN(FILE, X) fseek(FILE, 0, SEEK_END);\
X = ftell(FILE);\
fseek(FILE, 0, SEEK_SET)

#define COMMAND_SIZE 1024 * 4

typedef struct cbuild_conf {
    cbstr_t source;
    cbstr_t project;
    bool cache;
} cbuild_conf_t;

size_t next(const char *string, size_t start, size_t len) {
    size_t i = start;
    if (string[i] == 0x0A) {
        ++i;
    }

    if (string[i] == 0x0D) {
        ++i;
    }

    for (; i < len; ++i) {
        if (string[i] == ' ' || string[i] == 0x0A) {
            --i;
            break;
        }
    }

    return i - start;
}

cbuild_conf_t load_conf(char *buffer, size_t len) {
    cbuild_conf_t config;
    config.cache = false;
    bool has_source = false;
    bool has_proj = false;

    size_t pos = 0;
    size_t size = 0;

    while (1) {
        size = next(buffer, pos, len);

        if (size == 0) break;

        if (strncmp(&buffer[pos], "source", size) == 0) {
            if (!has_source) {
                pos += size + 2;
                size = next(buffer, pos, len);
                config.source = cbstr_from_cstr(&buffer[pos], size);
                has_source = true;
            } else {
                printf("[ERROR] Multiple definition of source\n");
            }
        } else if (strncmp(&buffer[pos], "project", size) == 0) {
            if (!has_proj) {
                pos += size + 2;
                size = next(buffer, pos, len);
                config.project = cbstr_from_cstr(&buffer[pos], size);
                has_proj = true;
            } else {
                printf("[ERROR] Multiple definition of project\n");
            }
        } else if (strncmp(&buffer[pos], "cache", size) == 0) {
                pos += size + 2;
                size = next(buffer, pos, len);
                config.cache = strncmp(&buffer[pos], "on", size) == 0;
        }

        pos += size + 2;
    }

    if (!has_proj || !has_source) {
        printf("[ERROR] not enough information specified in cbuild...\n");
        exit(1);
    }
    return config;
}

void free_conf(cbuild_conf_t *conf) {
    cbstr_free(&conf->source);
    cbstr_free(&conf->project);
}


void obj_sub(char *file, size_t len) {
    size_t end = strnlen(file, len);
    file[end-1] = 'o';
}

cbstr_t build_object_command(cbuild_conf_t *conf, cbstr_t *buffer, cbstr_t *file, cbstr_t *parent) {
    cbstr_clear(buffer);
    cbstr_concat_cstr(buffer, "gcc -g -c ", sizeof("gcc -g -c "));

    cbstr_concat(buffer, parent);
    cbstr_concat_cstr(buffer, "\\", sizeof("\\"));
    cbstr_concat(buffer, file);

    cbstr_t object = cbstr_from_cstr("obj\\", sizeof("obj\\"));
    cbstr_concat_slice(&object, parent, conf->source.len);

    CreateDirectoryA(object.data, NULL);

    if (!cbstr_cmp(parent, &conf->source)) {
        cbstr_concat_cstr(&object, "\\", sizeof("\\"));
    }

    cbstr_concat(&object, file);
    object.data[object.len-2] = 'o';

    cbstr_concat_cstr(buffer, " -o ", sizeof(" -o "));
    cbstr_concat(buffer, &object);

    return object;
}

void compile(cbuild_conf_t *conf, dir_t *files) {
    #define FREE_ALL() cbstr_list_free(&objects);\
    cbstr_free(&command);\
    cbstr_free(&temp)

    cbstr_list_t objects = cbstr_list_init(files->entries.len >> 1);
    size_t pos;
    size_t i;
    int ret_val;
    cbstr_t temp;

    cbstr_t command = cbstr_with_cap(COMMAND_SIZE);

    CreateDirectoryA(".cbuild", NULL);

    for (i = 0; i < files->entries.len; ++i) {
        cbstr_t *parent;
        cbstr_t object;
        dir_entry_t *file = entry_list_get(&files->entries, i);
        cbstr_t *name = &file->filename;

        if (name->data[name->len-2] != 'c') continue;

        parent = cbstr_list_get(&files->dir_names, file->parent);

        object = build_object_command(conf, &command, name, parent);
        cbstr_list_push(&objects, object);

        printf("[CMD] %s\n", command.data);
        ret_val = system(command.data);

        if (ret_val != 0) {
            printf("[ERROR] '%s' failed with code %d!\n", command, ret_val);
            FREE_ALL();
            return;
        }
    }

    cbstr_concat_cstr(&conf->project, ".exe", sizeof(".exe"));
    temp = cbstr_from_cstr(".cbuild\\", sizeof(".cbuild\\"));
    cbstr_concat(&temp, &conf->project);
    cbstr_concat_cstr(&temp, ".tmp", sizeof(".tmp"));

    if (conf->cache) {
        printf("[INFO] Relocating .exe to cache...\n");
        DeleteFileA(temp.data);
        MoveFileA(conf->project.data, temp.data);
    }

    cbstr_clear(&command);
    cbstr_concat_cstr(&command, "gcc -g -o ", sizeof("gcc -g -o "));
    cbstr_concat(&command, &conf->project);

    for (i = 0; i < objects.len; ++i) {
        cbstr_concat_cstr(&command, " ", sizeof(" "));
        cbstr_concat(&command, cbstr_list_get(&objects, i));
    }

    printf("[CMD] %s\n", command.data);
    ret_val = system(command.data);

    if (ret_val != 0) {
        printf("[ERROR] '%s' failed with code %d!\n", command, ret_val);
        MoveFileA(temp.data, conf->project.data);
    }

    FREE_ALL();
    #undef FREE_ALL
}

int main(void) {
    FILE *cbuild;
    size_t length;
    char *config_data;
    cbuild_conf_t config;

    DEBUG_INIT();

    cbuild = fopen("cbuild", "rb");
    if (!cbuild) {
        printf("[ERROR] Could not load cbuild config file...\n");
        exit(1);
    }

    FLEN(cbuild, length);
    config_data = (char*)MALLOC(length);
    fread(config_data, length, 1, cbuild);
    fclose(cbuild);

    config = load_conf(config_data, length);
    FREE(config_data);

    dir_t files = walk_dir(config.source);

    compile(&config, &files);

    free_conf(&config);
    free_dir(&files);
    
    DEBUG_DEINIT();
    return 0;
}
