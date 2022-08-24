//
// Created by LAMBDA on 2022/8/22.
//

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_t.h"

typedef struct scanner {
    size_t lineno; // current line number
    size_t column; // current line column
    size_t index;  // current buf index
    FILE *file;    // current file instance
    size_t len;
    char *buf; // buf data
} scanner;

int scanner_init_from_file_path(struct scanner *scanner, const char *file);

int scanner_init(struct scanner *scanner, char *source);

void scanner_finalize(struct scanner *scanner);

typedef struct parse_context_t {
    struct scanner scanner;
    struct json_string error_info;
} parse_context_t;

int parse_context_init(struct parse_context_t *ctx, char *source);

int parse_context_init_from_file_path(struct parse_context_t *ctx, const char *file);

int parse_context_init_from_file(struct parse_context_t *ctx, FILE *file);

int parse_context_finalize(struct parse_context_t *ctx);

int parse(struct parse_context_t *ctx, struct json_t *json);

#endif //JSON_PARSER_H
