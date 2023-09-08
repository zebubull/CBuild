#pragma once

#include <time.h>

#define TICKS_PER_SECOND 10000000
#define EPOCH_DIFFERENCE 11644473600LL

inline time_t filetime_to_time_t(long long ft) {
    time_t time;
    time = ft / TICKS_PER_SECOND;
    time = time - EPOCH_DIFFERENCE;
    return time;
}