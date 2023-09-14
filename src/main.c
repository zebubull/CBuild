/// Author - zebubull
/// main.c
/// cbuild main file.
/// Copyright (c) zebubull 2023

#include "./core/cbcore.h"

#include <stdio.h>

int main(int argc, char **argv) {
    printf("[INFO] CBuild version 0.0.3\n");
    return cb_main(argc, argv);
}
