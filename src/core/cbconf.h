/// Author - zebubull
/// cbconf.h
/// A header for loading a cbuild config file.
/// Copyright (c) zebubull 2023
#pragma once

#include "../util/cbstr.h"

typedef struct cbconf {
    cbstr_t source;
    cbstr_t project;
    cbstr_t rule;
    cbstr_list_t defines;
    cbstr_list_t flags;
    bool cache;
} cbconf_t;

cbconf_t cbconf_init(char *buffer, size_t len, int argc, char **argv);
void cbconf_free(cbconf_t *conf);
