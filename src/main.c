#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>

#include "dir.h"
#include "cbmem.h"
#include "cbtimetable.h"

#define FLEN(FILE, X) fseek(FILE, 0, SEEK_END);\
X = ftell(FILE);\
fseek(FILE, 0, SEEK_SET)

#define COMMAND_SIZE 1024 * 4

static time_table_t timetable;

typedef struct cbuild_conf {
    cbstr_t source;
    cbstr_t project;
    cbstr_t rule;
    cbstr_list_t defines;
    cbstr_list_t flags;
    bool cache;
} cbuild_conf_t;

size_t next(const char *string, size_t *start, size_t len) {
    size_t i = *start;
    while (i < len && isspace(string[i])) {
        ++i;
    }

    *start = i;

    for (; i < len; ++i) {
        if (string[i] == ' ' || string[i] == 0x0A) {
            --i;
            break;
        }
    }

    return i - *start;
}

cbuild_conf_t load_conf(char *buffer, size_t len, int argc, char **argv) {
    cbstr_t rule;
    cbuild_conf_t config;
    config.cache = false;
    config.defines = cbstr_list_init(4);
    config.flags = cbstr_list_init(4);
    bool has_source = false;
    bool has_proj = false;
    bool ignore_rule = false;

    if (argc > 1) {
        rule = cbstr_from_cstr(argv[1], strnlen(argv[1], 64));
    } else {
        rule = cbstr_from_cstr("default", sizeof("default"));
    }

    size_t pos = 0;
    size_t size = 0;

    while (1) {
        size = next(buffer, &pos, len);

        if (size == 0) break;

        if (strncmp(&buffer[pos], "rule", size) == 0) {
            pos += size + 1;
            size = next(buffer, &pos, len);
            if (strncmp(rule.data, "default", rule.len) == 0) {
                cbstr_clear(&rule);
                cbstr_concat_cstr(&rule, &buffer[pos], size);
            }
            ignore_rule = strncmp(&buffer[pos], rule.data, size) != 0;
        }

        if (strncmp(&buffer[pos], "endrule", size) == 0) {
            ignore_rule = false;
        }

        if (ignore_rule) {
            pos += size + 1;
            continue;
        }

        if (strncmp(&buffer[pos], "source", size) == 0) {
            if (!has_source) {
                pos += size + 1;
                size = next(buffer, &pos, len);
                config.source = cbstr_from_cstr(&buffer[pos], size);
                has_source = true;
            } else {
                printf("[ERROR] Multiple definition of source\n");
            }
        } else if (strncmp(&buffer[pos], "project", size) == 0) {
            if (!has_proj) {
                pos += size + 1;
                size = next(buffer, &pos, len);
                config.project = cbstr_from_cstr(&buffer[pos], size);
                has_proj = true;
            } else {
                printf("[ERROR] Multiple definition of project\n");
            }
        } else if (strncmp(&buffer[pos], "cache", size) == 0) {
            pos += size + 1;
            size = next(buffer, &pos, len);
            config.cache = strncmp(&buffer[pos], "on", size) == 0;
        } else if (strncmp(&buffer[pos], "define", size) == 0) {
            pos += size + 1;
            size = next(buffer, &pos, len);
            cbstr_list_push(&config.defines, cbstr_from_cstr(&buffer[pos], size));
        } else if (strncmp(&buffer[pos], "flag", size) == 0) {
            pos += size + 1;
            size = next(buffer, &pos, len);
            cbstr_list_push(&config.flags, cbstr_from_cstr(&buffer[pos], size));
        }

        pos += size + 1;
    }

    if (!has_proj || !has_source) {
        printf("[ERROR] not enough information specified in cbuild...\n");
        exit(1);
    }

    config.rule = rule;

    return config;
}

void free_conf(cbuild_conf_t *conf) {
    cbstr_free(&conf->source);
    cbstr_free(&conf->project);
    cbstr_free(&conf->rule);
    cbstr_list_free(&conf->defines);
    cbstr_list_free(&conf->flags);
}


cbstr_t build_object_command(cbuild_conf_t *conf, cbstr_t *buffer, cbstr_t *file, cbstr_t *parent) {
    size_t i;
    cbstr_t object;

    cbstr_clear(buffer);
    cbstr_concat_cstr(buffer, "gcc -c ", sizeof("gcc -c "));

    for (i = 0; i < conf->defines.len; ++i) {
        cbstr_concat_format(buffer, CB_CSTR("-D%s "), cbstr_list_get(&conf->defines, i));
    }

    for (i = 0; i < conf->flags.len; ++i) {
        cbstr_concat_format(buffer, CB_CSTR("%s "), cbstr_list_get(&conf->flags, i));
    }

    cbstr_concat_format(buffer, CB_CSTR("%s\\%s "), parent, file);

    object = cbstr_with_cap(conf->source.len);
    cbstr_concat_format(&object, CB_CSTR("obj\\%s\\"), &conf->rule);
    cbstr_concat_slice(&object, parent, conf->source.len);

    CreateDirectoryA(object.data, NULL);

    // Avoid double slash in obj path since source directory is cut out
    if (!cbstr_cmp(parent, &conf->source)) {
        cbstr_concat_cstr(&object, "\\", sizeof("\\"));
    }

    // Bad hack, fix later
    cbstr_concat(&object, file);
    object.data[object.len-2] = 'o';

    cbstr_concat_format(buffer, CB_CSTR("-o %s"), &object);

    return object;
}

bool needs_compile(cbstr_t *object, dir_entry_t *file, cbstr_t *parent, time_entry_t **entry) {
    *entry = time_table_search(&timetable, &file->filename, parent);

    if (!(*entry)) {
        return true;
    }

    if (file->write_time > (*entry)->write_time) {
        return true;
    }

    DWORD attr = GetFileAttributesA(object->data);
    return attr == INVALID_FILE_ATTRIBUTES;
}

void compile(cbuild_conf_t *conf, dir_t *files) {
    #define FREE_ALL() cbstr_list_free(&objects);\
    cbstr_free(&command);\
    cbstr_free(&temp)

    cbstr_list_t objects = cbstr_list_init(files->entries.len >> 1);
    size_t i;
    int ret_val;
    cbstr_t temp;
    bool built = false;

    cbstr_t command = cbstr_with_cap(COMMAND_SIZE);

    CreateDirectoryA(".cbuild", NULL);

    for (i = 0; i < files->entries.len; ++i) {
        cbstr_t *parent;
        cbstr_t object;
        time_entry_t *pentry;
        dir_entry_t *file = entry_list_get(&files->entries, i);
        cbstr_t *name = &file->filename;

        if (name->data[name->len-2] != 'c') continue;

        parent = cbstr_list_get(&files->dir_names, file->parent);

        object = build_object_command(conf, &command, name, parent);

        if (!needs_compile(&object, file, parent, &pentry)) {
            printf("[INFO] %s\\%s up to date\n", parent->data, name->data);
            // Use cached obj file
            cbstr_list_push(&objects, cbstr_copy(&pentry->obj));
            cbstr_free(&object);
            continue;
        }

        cbstr_list_push(&objects, object);
        built = true;

        printf("[CMD] %s\n", command.data);
        ret_val = system(command.data);

        if (ret_val == 0) {
            if (pentry == NULL) {
                time_entry_t entry;
                entry.file = cbstr_copy(name);
                entry.parent = cbstr_copy(parent);
                entry.obj = cbstr_copy(&object);
                entry.write_time = file->write_time;

                time_table_push(&timetable, entry);
            } else {
                cbstr_free(&pentry->obj);
                pentry->obj = cbstr_copy(&object);
                pentry->write_time = file->write_time;
            }
        } else {
            printf("[ERROR] '%s' failed with code %d!\n", command.data, ret_val);
            FREE_ALL();
            return;
        }
    }


    cbstr_concat_format(&conf->project, CB_CSTR("-%s.exe"), &conf->rule);
    temp = cbstr_with_cap(conf->rule.len + 16);
    cbstr_concat_format(&temp, CB_CSTR(".cbuild\\%s.tmp"), &conf->project);

    if (!built) {
        printf("[INFO] %s up to date\n", conf->project.data);
        FREE_ALL();
        return;
    }

    if (conf->cache) {
        printf("[INFO] Relocating .exe to cache...\n");
        DeleteFileA(temp.data);
        MoveFileA(conf->project.data, temp.data);
    }

    cbstr_clear(&command);
    cbstr_concat_format(&command, CB_CSTR("gcc -g -o %s "), &conf->project);

    for (i = 0; i < objects.len; ++i) {
        cbstr_concat_format(&command, CB_CSTR("%s "), cbstr_list_get(&objects, i));
    }

    printf("[CMD] %s\n", command.data);
    ret_val = system(command.data);

    if (ret_val != 0) {
        printf("[ERROR] '%s' failed with code %d!\n", command.data, ret_val);
        MoveFileA(temp.data, conf->project.data);
    }

    FREE_ALL();
    #undef FREE_ALL
}

int main(int argc, char **argv) {
    FILE *cbuild;
    FILE *tt;
    size_t length;
    char *config_data;
    cbuild_conf_t config;
    cbstr_t tt_file;

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

    config = load_conf(config_data, length, argc, argv);
    FREE(config_data);

    dir_t files = walk_dir(config.source);

    timetable = time_table_init(4);

    tt_file = cbstr_with_cap(16);
    cbstr_concat_format(&tt_file, CB_CSTR(".cbuild\\%s-timetable"), &config.rule);
    tt = fopen(tt_file.data, "r");

    if (tt) {
        time_table_load(&timetable, tt);
        if (tt) fclose(tt);
    }

    compile(&config, &files);

    tt = fopen(tt_file.data, "w");
    cbstr_free(&tt_file);

    if (tt) {
        time_table_save(&timetable, tt);
        fclose(tt);
    }

    free_conf(&config);
    free_dir(&files);
    time_table_free(&timetable);
    
    DEBUG_DEINIT();
    return 0;
}
