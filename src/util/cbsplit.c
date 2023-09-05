/// Author - zebubull
/// cbsplit.c
/// cbsplit.h implementation.
/// Copyright (c) zebubull 2023

#include "cbsplit.h"
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>

bool cbsplit_next(cbsplit_t *view) {
    size_t i = 0;
    size_t start = 0;

    view->data += view->len;
    view->remaining -= view->len;

    while (start < view->remaining && isspace(view->data[start])) {
        ++start;
    }

    view->data += start;
    view->remaining -= start;

    if (view->remaining == 0) {
        return false;
    }

    while (i < view->remaining && !isspace(view->data[i])) {
        ++i;
    }

    view->len = i;
    return true;
}

cbsplit_t cbsplit_init(char *str, size_t len) {
    cbsplit_t view;
    view.data = str;
    view.len = 0;
    view.remaining = len;

    return view;
}