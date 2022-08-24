//
// Created by LAMBDA on 2022/8/20.
//

#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stddef.h>

static size_t next_bin_exp(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return ++n;
}

#endif //JSON_UTIL_H
