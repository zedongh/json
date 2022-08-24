#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include "json_t.h"
#include "util.h"

#define DEFINE_ENSURE_TYPE_BUFFER_CAPACITY(type, elem_type) \
size_t ensure_ ## type ## _capacity(type *buffer, size_t more) { \
    if (buffer->len + more < buffer->cap) {          \
        return buffer->cap;                          \
    }                                                     \
    size_t new_cap = next_bin_exp(buffer->len + more);  \
    buffer->buf = (elem_type *)realloc(buffer->buf, sizeof(elem_type) * new_cap); \
    return buffer->cap = new_cap;               \
}

DEFINE_ENSURE_TYPE_BUFFER_CAPACITY(json_array, json_t)

DEFINE_ENSURE_TYPE_BUFFER_CAPACITY(json_string, char)

#undef DEFINE_ENSURE_TYPE_BUFFER_CAPACITY

#define DEFINE_INIT_TYPE_BUFFER(type, elem_type) \
void type ## _init_with_capacity(type *buffer, size_t capacity) { \
    buffer->len = 0;                             \
    buffer->cap = capacity;                 \
    buffer->buf = NULL;                       \
    if (buffer->cap > 0) {                  \
        buffer->buf = (elem_type *) realloc(buffer->buf, capacity * sizeof(elem_type)); \
    }                                            \
}                                                \
                                                 \
void type ## _init(type *buffer) {               \
    type ## _init_with_capacity(buffer, 0);  \
}

DEFINE_INIT_TYPE_BUFFER(json_string, char)

DEFINE_INIT_TYPE_BUFFER(json_array, json_t)

#undef DEFINE_INIT_TYPE_BUFFER

#define DEFINE_RELEASE_TYPE_BUFFER(type) \
void release_ ## type(type *buffer) {    \
    if (buffer->buf) {                   \
        free(buffer->buf);               \
    }                                    \
    buffer->buf = NULL;                  \
    buffer->cap = 0;                     \
    buffer->len = 0;                     \
}

DEFINE_RELEASE_TYPE_BUFFER(json_string)

DEFINE_RELEASE_TYPE_BUFFER(json_array)

#undef DEFINE_RELEASE_TYPE_BUFFER

struct json_t json_true = {.kind = JSON_TRUE};
struct json_t json_false = {.kind = JSON_FALSE};
struct json_t json_null = {.kind = JSON_NULL};

struct json_t json_number(double value) {
    struct json_t result = {
            .kind = JSON_NUMBER,
            .num_val = value,
    };
    return result;
}

struct json_t json_str(const char *cs) {
    struct json_string str;
    json_string_init_with_capacity(&str, strlen(cs) + 1);
    json_string_append(&str, cs);
    struct json_t result = {
            .kind = JSON_STRING,
            .str_val = str,
    };
    return result;
}

struct json_t empty_json_array() {
    struct json_array array;
    json_array_init(&array);
    struct json_t json = {
            .kind = JSON_ARRAY,
            .array_val = array,
    };
    return json;
}

struct json_t empty_json_object() {
    struct json_object object;
    json_object_init(&object);
    struct json_t json = {
            .kind = JSON_OBJECT,
            .object_val = object,
    };
    return json;
}


void json_init(struct json_t *json, enum json_kind kind) {
    json->kind = kind;
    switch (kind) {
        case JSON_NUMBER:
            json->num_val = 0;
            break;
        case JSON_STRING:
            json_string_init(&json->str_val);
            break;
        case JSON_ARRAY:
            json_array_init(&json->array_val);
            break;
        case JSON_OBJECT:
            json_object_init(&json->object_val);
            break;
        default:
            break;
    }
}

void json_finalize(struct json_t *json) {
    switch (json->kind) {
        case JSON_STRING:
            json_string_finalize(&json->str_val);
            break;
        case JSON_ARRAY:
            json_array_finalize(&json->array_val);
            break;
        case JSON_OBJECT:
            json_object_finalize(&json->object_val);
            break;
        default:
            break;
    }
}

void json_stringify(struct json_t *json, struct json_string *buf) {
    switch (json->kind) {
        case JSON_UNKNOWN:
            json_string_append(buf, "<unknown>");
            return;
        case JSON_NULL:
            json_string_append(buf, "null");
            return;
        case JSON_TRUE:
            json_string_append(buf, "true");
            return;
        case JSON_FALSE:
            json_string_append(buf, "false");
            return;
        case JSON_NUMBER: {
            char str[30];
            sprintf(str, "%g", json->num_val);
            json_string_append(buf, str);
            return;
        }
        case JSON_STRING: {
            json_string_append_char(buf, '"');
            if (json->str_val.buf) {
                json_string_append(buf, json->str_val.buf);
            }
            json_string_append_char(buf, '"');
            return;
        }
        case JSON_ARRAY: {
            json_string_append(buf, "[");
            for (int i = 0; i < json->array_val.len; ++i) {
                if (i != 0) {
                    json_string_append(buf, ", ");
                }
                json_stringify(json->array_val.buf + i, buf);
            }
            json_string_append(buf, "]");
            return;
        }
        default: {
            json_string_append(buf, "{");
            struct object_entry *entry_iter = json_object_entries(&json->object_val);
            bool first = true;
            while (entry_iter) {
                if (!first) {
                    json_string_append(buf, ", ");
                } else {
                    first = false;
                }
                struct json_t key;
                key.kind = JSON_STRING;
                key.str_val = entry_iter->key;
                json_stringify(&key, buf);
                json_string_append(buf, ": ");
                json_stringify(&entry_iter->value, buf);
                entry_iter = entry_iter->link_next;
            }
            json_string_append(buf, "}");
            return;
        }
    }
}

void json_array_finalize(struct json_array *array) {
    for (int i = 0; i < array->len; ++i) {
        json_finalize(array->buf + i);
    }
    release_json_array(array);
}

size_t json_array_append(struct json_array *array, struct json_t value) {
    ensure_json_array_capacity(array, 1);
    memcpy(array->buf + array->len, &value, sizeof(struct json_t));
    array->len += 1;
    return 0;
}

size_t json_string_append(struct json_string *buffer, const char *cs) {
    return json_string_n_append(buffer, cs, strlen(cs));
}

size_t json_string_n_append(struct json_string *buffer, const char *cs, size_t n) {
    ensure_json_string_capacity(buffer, n + 1);
    strncpy(buffer->buf + buffer->len, cs, n);
    buffer->len += n;
    buffer->buf[buffer->len] = '\0';
    return buffer->len;
}

size_t json_string_append_char(struct json_string *buffer, char ch) {
    ensure_json_string_capacity(buffer, 2);
    buffer->buf[buffer->len++] = ch;
    buffer->buf[buffer->len] = '\0';
    return buffer->len;
}

void json_string_finalize(struct json_string *str) {
    release_json_string(str);
}