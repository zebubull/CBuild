/// Author - zebubull
/// cbconf.c
/// cbconf.h implementation.
/// Copyright (c) zebubull 2023

#include "cbconf.h"
#include "../util/cbstr.h"
#include "../util/cbsplit.h"
#include "../util/cblog.h"

#include <stdlib.h>

cbconf_t cbconf_init(char *buffer, size_t len, int argc, char **argv) {
    cbstr_t rule;
    cbsplit_t view;
    cbconf_t config;
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

    view = cbsplit_init(buffer, len);

    while (cbsplit_next(&view)) {

        if (strncmp("rule", view.data, view.len) == 0) {
            if (!cbsplit_next(&view)) {
                eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                exit(1);
            }

            if (strncmp(rule.data, "default", rule.len) == 0) {
                cbstr_clear(&rule);
                cbstr_concat_cstr(&rule, view.data, view.len);
                ignore_rule = false;
            } else {
                ignore_rule = strncmp(rule.data, view.data, view.len) != 0;
            }
            continue;
        }

        if (strncmp("endrule", view.data, view.len) == 0) {
            ignore_rule = false;
            continue;
        }

        if (ignore_rule) {
            continue;
        }

        if (strncmp("source", view.data, view.len) == 0) {
            if (!has_source) {
                if (!cbsplit_next(&view)) {
                    eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                    exit(1);
                }

                config.source = cbstr_from_cstr(view.data, view.len);
                has_source = true;
            } else {
                eprintf("[ERROR] Multiple definition of source\n");
                exit(1);
            }
        } else if (strncmp("project", view.data, view.len) == 0) {
            if (!has_proj) {
                if (!cbsplit_next(&view)) {
                    eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                    exit(1);
                }

                config.project = cbstr_from_cstr(view.data, view.len);
                has_proj = true;
            } else {
                eprintf("[ERROR] Multiple definition of project\n");
                exit(1);
            }
        } else if (strncmp("cache", view.data, view.len) == 0) {
            if (!cbsplit_next(&view)) {
                eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                exit(1);
            }
            
            if (strncmp("on", view.data, view.len) == 0) {
                config.cache = true;
            } else if (strncmp("off", view.data, view.len) == 0) {
                config.cache = false;
            } else {
                eprintf("[ERROR] Unknown cache mode in cbuild conf.\n");
                exit(1);
            }
        } else if (strncmp("define", view.data, view.len) == 0) {
            if (!cbsplit_next(&view)) {
                eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                exit(1);
            }

            cbstr_list_push(&config.defines, cbstr_from_cstr(view.data, view.len));
        } else if (strncmp("flag", view.data, view.len) == 0) {
            if (!cbsplit_next(&view)) {
                eprintf("[ERROR] Unexpected EOS in cbuild conf.\n");
                exit(1);
            }

            cbstr_list_push(&config.flags, cbstr_from_cstr(view.data, view.len));
        } else {
            char cache = view.data[view.len];
            view.data[view.len] = 0;
            eprintf("[WARNING] Ignoring unknown directive '%s'\n", view.data);
            view.data[view.len] = cache;
        }
    }

    if (!has_proj || !has_source) {
        eprintf("[ERROR] not enough information specified in cbuild...\n");
        exit(1);
    }

    config.rule = rule;
    return config;
}

void cbconf_free(cbconf_t *conf) {
    cbstr_free(&conf->source);
    cbstr_free(&conf->project);
    cbstr_free(&conf->rule);
    cbstr_list_free(&conf->defines);
    cbstr_list_free(&conf->flags);
}
