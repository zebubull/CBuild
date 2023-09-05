#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <stdbool.h>

#include "cbcore.h"
#include "cbconf.h"
#include "../os/dir.h"
#include "../mem/cbmem.h"
#include "../util/cbtimetable.h"
#include "../util/cbstr.h"
#include "../util/cblog.h"

#define FLEN(FILE, X) fseek(FILE, 0, SEEK_END);\
X = ftell(FILE);\
fseek(FILE, 0, SEEK_SET)

#define COMMAND_SIZE 1024 * 4

static time_table_t timetable;

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

void set_compiler_stub(cbconf_t *conf, cbstr_t *str) {
    size_t i;

    cbstr_concat_cstr(str, CB_CSTR("gcc -c "));

    for (i = 0; i < conf->defines.len; ++i) {
        cbstr_concat_format(str, CB_CSTR("-D%s "), cbstr_list_get(&conf->defines, i));
    }

    for (i = 0; i < conf->flags.len; ++i) {
        cbstr_concat_format(str, CB_CSTR("%s "), cbstr_list_get(&conf->flags, i));
    }
}

void compile(cbconf_t *conf, dir_t *files) {
    #define FREE_ALL() cbstr_list_free(&objects);\
    cbstr_free(&command);\
    cbstr_free(&temp)

    size_t i;
    int ret_val;
    cbstr_t temp;
    size_t stub_len;
    cbstr_list_t objects = cbstr_list_init(files->entries.len >> 1);
    bool built = false;

    CreateDirectoryA(".cbuild", NULL);

    cbstr_t command = cbstr_with_cap(COMMAND_SIZE);
    set_compiler_stub(conf, &command);
    stub_len = command.len;

    for (i = 0; i < files->entries.len; ++i) {
        cbstr_t *parent;
        cbstr_t object;
        cbstr_t path;
        time_entry_t *pentry;

        dir_entry_t *file = entry_list_get(&files->entries, i);
        cbstr_t *name = &file->filename;

        if (name->data[name->len-2] != 'c') continue;

        parent = cbstr_list_get(&files->dir_names, file->parent);

        path = cbstr_copy(parent);
        cbstr_concat_format(&path, CB_CSTR("\\%s"), name);

        object = cbstr_with_cap(32);
        cbstr_concat_format(&object, CB_CSTR("obj\\%s\\"), conf->rule);
        cbstr_concat_slice(&object, parent, conf->source.len);
        cbstr_concat_format(&object, CB_CSTR("\\%s"), name);

        object.data[object.len-2] = 'o';

        command.len = stub_len;
        cbstr_concat_format(&command, CB_CSTR("%s -o %s"), &path, &object);

        if (!needs_compile(&object, file, parent, &pentry)) {
            printf("[INFO] %s up to date\n", path.data);
            cbstr_list_push(&objects, cbstr_copy(&pentry->obj));
            cbstr_free(&object);
            cbstr_free(&path);
            continue;
        }

        cbstr_free(&path);
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
            eprintf("[ERROR] '%s' failed with code %d!\n", command.data, ret_val);
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
        eprintf("[ERROR] '%s' failed with code %d!\n", command.data, ret_val);
        MoveFileA(temp.data, conf->project.data);
    }

    FREE_ALL();
    #undef FREE_ALL
}

cbconf_t load_config(int argc, char **argv) {
    FILE *config_file;
    size_t data_size;
    char *config_data;
    cbconf_t config;

    config_file = fopen("cbuild", "rb");
    if (!config_file) {
        eprintf("[ERROR] Failed to open cbuild config.\n");
        exit(1);
    }

    fseek(config_file, 0, SEEK_END);
    data_size = ftell(config_file);
    fseek(config_file, 0, SEEK_SET);

    config_data = MALLOC(data_size);
    fread(config_data, 1, data_size, config_file);
    fclose(config_file);

    config = cbconf_init(config_data, data_size, argc, argv);

    free(config_data);

    return config;
}

void load_timetable(cbstr_t path) {
    FILE *timetable_file;

    timetable = time_table_init(4);

    timetable_file = fopen(path.data, "r");

    if (timetable_file) {
        time_table_load(&timetable, timetable_file);
        fclose(timetable_file);
    } else {
        printf("[WARNING] Could not open timetable file.\n");
    }
}

void save_timetable(cbstr_t path) {
    FILE *timetable_file;

    timetable_file = fopen(path.data, "w");

    if (timetable_file) {
        time_table_save(&timetable, timetable_file);
        fclose(timetable_file);
    } else {
        printf("[WARNING] Could not open timetable file.\n");
    }
}

int cb_main(int argc, char **argv) {
    cbconf_t config;
    dir_t files;
    cbstr_t timetable_path;

    DEBUG_INIT();

    config = load_config(argc, argv);
    files = walk_dir(config.source);

    timetable_path = cbstr_with_cap(19 + config.rule.len);
    cbstr_concat_format(&timetable_path, CB_CSTR(".cbuild\\%s-timetable"), &config.rule);

    load_timetable(timetable_path);

    compile(&config, &files);

    save_timetable(timetable_path);

    time_table_free(&timetable);
    cbstr_free(&timetable_path);
    dir_free(&files);
    cbconf_free(&config);
    
    DEBUG_DEINIT();

    return 0;
}
