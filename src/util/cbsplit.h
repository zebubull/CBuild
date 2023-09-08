/// Author - zebubull
/// cbsplit.h
/// A simple header for whitespace-delimited string splitting
/// Copyright (c) zebubull 2023
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct cbsplit {
    char *data;
    size_t len;
    size_t remaining;
} cbsplit_t;

cbsplit_t cbsplit_init(char *str, size_t len);

// This function will advace the data pointer forward to the next word and 
// update the length of the current word.
bool cbsplit_next(cbsplit_t *view);
