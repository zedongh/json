//
// Created by LAMBDA on 2022/8/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage ./json <file>");
        exit(-1);
    }
    char * path = argv[1];

    struct parse_context_t ctx;
    int r = parse_context_init_from_file_path(&ctx, path);
    if (r) {
        fprintf(stderr, "error happened when load file");
        parse_context_finalize(&ctx);
        exit(-1);
    }
    struct json_t json;
    struct timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);
    r = parse(&ctx, &json);
    clock_gettime(CLOCK_REALTIME, &stop);
    if (r) {
        fprintf(stderr, "error: %s", ctx.error_info.buf);
        json_finalize(&json);
        parse_context_finalize(&ctx);
        exit(-1);
    }
    printf("time used: %lld ms",
           ((unsigned long long )(stop.tv_sec - start.tv_sec) * (unsigned long )(1000000000L) +
           (unsigned long long )(stop.tv_nsec - start.tv_nsec)) / 1000000L
           );
    json_finalize(&json);
    parse_context_finalize(&ctx);
    return 0;
}