//
// Created by LAMBDA on 2022/8/22.
//

#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

#define K_NULL "null"
#define K_TRUE "true"
#define K_FALSE "false"
#define K_NULL_LEN 4
#define K_TRUE_LEN 4
#define K_FALSE_LEN 5

#define CHUNK_SIZE 8192

#define DEFINE_PARSE_JSON_TYPE(type) \
int parse_ ## type(struct parse_context_t *ctx, struct json_t *json);

DEFINE_PARSE_JSON_TYPE(json_array)

DEFINE_PARSE_JSON_TYPE(json_string)

DEFINE_PARSE_JSON_TYPE(json_object)

DEFINE_PARSE_JSON_TYPE(json_null)

DEFINE_PARSE_JSON_TYPE(json_false)

DEFINE_PARSE_JSON_TYPE(json_true)

DEFINE_PARSE_JSON_TYPE(json_number)

#undef DEFINE_PARSE_JSON_TYPE

#define char_buffer json_string
#define char_buffer_push json_string_append_char
#define char_buffer_init json_string_init
#define char_buffer_finalize json_string_finalize


char current_char(struct scanner *scanner) {
    return scanner->buf[scanner->index];
}

void advance(struct scanner *scanner) {
    char current = current_char(scanner);
    if (scanner->index + 1 >= CHUNK_SIZE) {
        memset(scanner->buf, '\0', CHUNK_SIZE);
        scanner->len = fread(scanner->buf, sizeof(char), CHUNK_SIZE, scanner->file);
        scanner->index = 0;
    } else if (scanner->index + 1 >= scanner->len) {
        scanner->index = scanner->len;
        return;
    } else {
        scanner->index++;
    }
    scanner->column++;
    if (current == '\n') {
        scanner->lineno++;
        scanner->column = 0;
    }
}

void scanner_finalize(struct scanner *scanner) {
    if (scanner->file) {
        fclose(scanner->file);
    }
    scanner->file = NULL;
    if (scanner->buf) {
        free(scanner->buf);
    }
    scanner->buf = NULL;
    scanner->lineno = 1;
    scanner->column = 0;
    scanner->index = 0;
}

static int init_scanner_from_file(struct scanner *scanner, FILE *file) {
    if (!file) {
        return -1;
    }
    scanner->lineno = 1;
    scanner->column = 0;
    scanner->index = 0;
    scanner->len = 0;
    scanner->file = file;
    scanner->buf = (char *) calloc(CHUNK_SIZE, sizeof(char));
    size_t len;
    if ((len = fread(scanner->buf, sizeof(char), CHUNK_SIZE, scanner->file)) == 0) {
        return -1;
    }
    scanner->len = len;
    return 0;
}

int scanner_init(struct scanner *scanner, char *source) {
    FILE *file = fmemopen(source, strlen(source), "r");
    return init_scanner_from_file(scanner, file);
}

int scanner_init_from_file_path(struct scanner *scanner, const char *file) {
    FILE *f = fopen(file, "r");
    return init_scanner_from_file(scanner, f);
}


void skip_whitespace(struct parse_context_t *ctx) {
    while (isspace(current_char(&ctx->scanner))) {
        advance(&ctx->scanner);
    }
}

int expect_char(struct parse_context_t *ctx, const char expected) {
    char ch = current_char(&ctx->scanner);
    if (ch != expected) {
        char error[200] = {0};
        sprintf(error, "[%zu:%zu] expect character %x but got character %x",
                ctx->scanner.lineno, ctx->scanner.column, expected, ch);
        json_string_append(&ctx->error_info, error);
        return -1;
    }
    advance(&ctx->scanner);
    return 0;
}

bool iseof(const char *cs) {
    if (cs == NULL || *cs == '\0') {
        return true;
    }
    return false;
}

bool lookahead(struct parse_context_t *ctx, const char ch) {
    return current_char(&ctx->scanner) == ch;
}

int must_done(struct parse_context_t *ctx) {
    skip_whitespace(ctx);
    return expect_char(ctx, '\0');
}

int parse_literal(
        struct parse_context_t *ctx, struct json_t *json, enum json_kind kind, const char *literal, size_t len) {
    int result = 0;
    for (int i = 0; i < len; ++i) {
        if ((result = expect_char(ctx, literal[i])) != 0) {
            char error[100] = {0};
            sprintf(error, "[%zu, %zu] parse literal value \"%s\" mismatch at character '%c' got '%c'",
                    ctx->scanner.lineno, ctx->scanner.column, literal, literal[i], current_char(&ctx->scanner));
            json_string_n_append(&ctx->error_info, error, strlen(error));
            return result;
        }
    }
    json->kind = kind;
    return result;
}

int parse_json_null(struct parse_context_t *ctx, struct json_t *json) {
    return parse_literal( ctx, json, JSON_NULL, K_NULL, K_NULL_LEN);
}

int parse_json_true(struct parse_context_t *ctx, struct json_t *json) {
    return parse_literal( ctx, json, JSON_TRUE, K_TRUE, K_TRUE_LEN);
}

int parse_json_false(struct parse_context_t *ctx, struct json_t *json) {
    return parse_literal( ctx, json, JSON_FALSE, K_FALSE, K_FALSE_LEN);
}

int parse_json_number(struct parse_context_t *ctx, struct json_t *json) {
    struct char_buffer buffer;
    char_buffer_init(&buffer);
    char ch;
    if ((ch = current_char(&ctx->scanner)) == '-') {
        advance(&ctx->scanner);
        char_buffer_push(&buffer, ch);
    }
    if (!isdigit(current_char(&ctx->scanner))) {
        goto catch;
    }
    if ((ch = current_char(&ctx->scanner)) == '0') {
        char_buffer_push(&buffer, ch);
        advance(&ctx->scanner);
    } else {
        if (!isdigit(current_char(&ctx->scanner))) {
            goto catch;
        }
        while (isdigit(ch = current_char(&ctx->scanner))) {
            char_buffer_push(&buffer, ch);
            advance(&ctx->scanner);
        }
    }
    if ((ch = current_char(&ctx->scanner)) == '.') {
        char_buffer_push(&buffer, ch);
        advance(&ctx->scanner);
        if (!isdigit(current_char(&ctx->scanner))) {
            goto catch;
        }
        while (isdigit(ch = current_char(&ctx->scanner))) {
            char_buffer_push(&buffer, ch);
            advance(&ctx->scanner);
        }
    }
    if ((ch = current_char(&ctx->scanner)) == 'e' || ch == 'E') {
        char_buffer_push(&buffer, ch);
        advance(&ctx->scanner);
        if ((ch = current_char(&ctx->scanner)) == '+' || ch == '-') {
            char_buffer_push(&buffer, ch);
            advance(&ctx->scanner);
        }
        if (!isdigit(current_char(&ctx->scanner))) {
            goto catch;
        }
        while (isdigit(ch = current_char(&ctx->scanner))) {
            char_buffer_push(&buffer, ch);
            advance(&ctx->scanner);
        }
    }
    json->kind = JSON_NUMBER;
    json->num_val = strtod(buffer.buf, NULL);
    char_buffer_finalize(&buffer);
    return 0;
catch:
    {
        char error[100] = {0};
        sprintf(error, "[%zu:%zu] json number require format -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)",
                ctx->scanner.lineno, ctx->scanner.column);
        json_string_n_append(&ctx->error_info, error, strlen(error));
        return -1;
    }
}

int _parse(struct parse_context_t *ctx, struct json_t *json, bool must) {
    if (current_char(&ctx->scanner) == '\0') {
        char info[100] = {0};
        sprintf(info, "[%zu:%zu] unexpected eof", ctx->scanner.lineno, ctx->scanner.column);
        json_string_append(&ctx->error_info, info);
        return -1;
    }
    skip_whitespace(ctx);
    int result = 0;
    switch (current_char(&ctx->scanner)) {
        case 'n':
            result = parse_json_null(ctx, json);
            break;
        case 'f':
            result = parse_json_false(ctx, json);
            break;
        case 't':
            result = parse_json_true(ctx, json);
            break;
        case '[':
            result = parse_json_array(ctx, json);
            break;
        case '{':
            result = parse_json_object(ctx, json);
            break;
        case '"':
            result = parse_json_string(ctx, json);
            break;
        default:
            result = parse_json_number(ctx, json);
            break;
    }
    if (result != 0) {
        return result;
    }
    if (must) {
        return must_done(ctx);
    }
    return result;
}

int _parse_json_string(struct parse_context_t *ctx, struct json_t *json, bool must) {
    int result;
    if ((result = expect_char(ctx, '"')) != 0) {
        return result;
    }
    json_string str;
    json_string_init(&str);
    while (!lookahead(ctx, '\0') && !lookahead(ctx, '"')) {
        if (lookahead(ctx, '\\')) {
            advance(&ctx->scanner);
            if (lookahead(ctx, '\0')) {
                result = -1;
                goto catch;
            }
            switch (current_char(&ctx->scanner)) {
                case '"':
                    json_string_append_char(&str, '"');
                    break;
                case '\\':
                    json_string_append_char(&str, '\\');
                    break;
                case 'b':
                    json_string_append_char(&str, '\b');
                    break;
                case 'f':
                    json_string_append_char(&str, '\f');
                    break;
                case 'n':
                    json_string_append_char(&str, '\n');
                    break;
                case 'r':
                    json_string_append_char(&str, '\r');
                    break;
                case 't':
                    json_string_append_char(&str, '\t');
                    break;
                default:
                    result = -1;
                    goto catch;
            }
        } else {
            json_string_append_char(&str, current_char(&ctx->scanner));
        }
        advance(&ctx->scanner);
    }
    if ((result = expect_char(ctx, '"')) != 0) {
        goto catch;
    }
    json->kind = JSON_STRING;
    json->str_val = str;
    if (must) {
        return must_done(ctx);
    }
    return result;
catch:
    json_string_finalize(&str);
    return result;
}

int parse_json_string(struct parse_context_t *ctx, struct json_t *json) {
    return _parse_json_string(ctx, json, false);
}

int parse_json_array(struct parse_context_t *ctx, struct json_t *json) {
    int result = 0;
    if ((result = expect_char(ctx, '[')) != 0) {
        return result;
    }
    json->kind = JSON_ARRAY;
    json_array_init(&json->array_val);
    if (!lookahead(ctx, ']')) {
        struct json_t elem;
        if ((result = _parse(ctx, &elem, false)) != 0) {
            return result;
        }
        json_array_append(&json->array_val, elem);
        skip_whitespace(ctx);
    }
    while (lookahead(ctx, ',')) {
        expect_char(ctx, ',');
        struct json_t elem;
        if ((result = _parse(ctx, &elem, false)) != 0) {
            return result;
        }
        json_array_append(&json->array_val, elem);
        skip_whitespace(ctx);
    }
    // lookahead
    if ((result = expect_char(ctx, ']')) != 0) {
        return result;
    }
    return result;
}

int parse_json_object_entry(struct parse_context_t *ctx, struct object_entry *entry) {
    int result;
    struct json_t key;
    json_init(&key, 0);
    if ((result = _parse_json_string(ctx, &key, false)) != 0) {
        return result;
    }
    skip_whitespace(ctx);
    if ((result = expect_char(ctx, ':')) != 0) {
        return result;
    }
    struct json_t value;
    json_init(&value, 0);
    if ((result = _parse(ctx, &value, false)) != 0) {
        return result;
    }
    entry->key = key.str_val;
    entry->value = value;
    return result;
}

int parse_json_object(struct parse_context_t *ctx, struct json_t *json) {
    int result;
    if ((result = expect_char(ctx, '{')) != 0) {
        return result;
    }
    skip_whitespace(ctx);
    json->kind = JSON_OBJECT;
    json_object_init(&json->object_val);
    if (!lookahead(ctx, '}')) {
        struct object_entry entry;
        if ((result = parse_json_object_entry(ctx, &entry)) != 0) {
            return result;
        }
        json_object_put(&json->object_val, entry.key, entry.value);
        skip_whitespace(ctx);
    }
    while (lookahead(ctx, ',')) {
        expect_char(ctx, ',');
        skip_whitespace(ctx);
        struct object_entry entry;
        if ((result = parse_json_object_entry(ctx, &entry)) != 0) {
            return result;
        }
        json_object_put(&json->object_val, entry.key, entry.value);
        skip_whitespace(ctx);
    }
    if ((result = expect_char(ctx, '}')) != 0) {
        return result;
    }
    return result;
}

int parse(struct parse_context_t *ctx, struct json_t *json) {
    return _parse(ctx, json, true);
}

int parse_context_init(struct parse_context_t *ctx, char *source) {
    FILE *f = fmemopen(source, strlen(source), "r");
    return parse_context_init_from_file(ctx, f);
}

int parse_context_init_from_file(struct parse_context_t *ctx, FILE *file) {
    struct json_string error_info;
    json_string_init(&error_info);
    struct scanner scanner;
    init_scanner_from_file(&scanner, file);
    ctx->error_info = error_info;
    ctx->scanner = scanner;
    return 0;
}

int parse_context_init_from_file_path(struct parse_context_t *ctx, const char *file) {
    FILE *f = fopen(file, "r");
    if (!f) {
        return -1;
    }
    return parse_context_init_from_file(ctx, f);
}


int parse_context_finalize(struct parse_context_t *ctx) {
    scanner_finalize(&ctx->scanner);
    json_string_finalize(&ctx->error_info);
    return 0;
}

#undef char_buffer_finalize
#undef char_buffer_init
#undef char_buffer_push
#undef char_buffer
