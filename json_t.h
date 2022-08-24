#ifndef __JSON_H__
#define __JSON_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


#define DECLARE_TYPE_BUFFER(elem_type, name) \
typedef struct name {                        \
  size_t len;                                \
  size_t cap;                                \
  elem_type *buf;                            \
} name;

DECLARE_TYPE_BUFFER(char, json_string)
DECLARE_TYPE_BUFFER(struct json_t, json_array)

#undef DECLARE_TYPE_BUFFER

#define DECLARE_INIT_TYPE_BUFFER(type) \
void type ## _init(type *buffer);      \
void type ## _init_with_capacity(type *buffer, size_t capacity);

DECLARE_INIT_TYPE_BUFFER(json_array)

DECLARE_INIT_TYPE_BUFFER(json_string)

#undef DECLARE_INIT_TYPE_BUFFER

#define DECLARE_APPEND_TYPE_BUFFER(type, elem_type) size_t type ## _append(type *buffer, elem_type value);

DECLARE_APPEND_TYPE_BUFFER(json_string, const char *)

DECLARE_APPEND_TYPE_BUFFER(json_array, struct json_t)

#undef DECLARE_APPEND_TYPE_BUFFER

#define DECLARE_FINALIZE_TYPE_BUFFER(type) void type ## _finalize(type *buffer);

DECLARE_FINALIZE_TYPE_BUFFER(json_string)

DECLARE_FINALIZE_TYPE_BUFFER(json_array)

#undef DECLARE_FINALIZE_TYPE_BUFFER


typedef struct json_object {
    struct object_entry **slots;
    struct object_entry *head;
    struct object_entry *tail;
    size_t len;
    size_t cap;
} json_object;

typedef enum json_kind {
    JSON_UNKNOWN,
    JSON_NULL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} json_kind;

typedef struct json_t {
    json_kind kind;
    union {
        double num_val;
        struct json_string str_val;
        struct json_array array_val;
        struct json_object object_val;
    };
} json_t;

typedef struct object_entry {
    struct json_string key;
    struct json_t value;
    struct object_entry *next;
    struct object_entry *link_prev;
    struct object_entry *link_next;
} object_entry;

extern struct json_t json_true;
extern struct json_t json_false;
extern struct json_t json_null;

struct json_t json_str(const char *cs);

struct json_t json_number(double value);

struct json_t empty_json_array();

struct json_t empty_json_object();

void json_init(struct json_t *json, enum json_kind kind);

void json_finalize(struct json_t *json);

size_t json_string_append_char(struct json_string *buffer, char ch);

size_t json_string_n_append(struct json_string *buffer, const char *cs, size_t n);

void json_stringify(struct json_t *json, struct json_string *buf);

void json_object_init(struct json_object *object);

void json_object_finalize(struct json_object *object);

void json_object_put(struct json_object *object, struct json_string key, struct json_t value);

struct json_t *json_object_get(struct json_object *object, struct json_string key);

struct object_entry *json_object_entries(struct json_object *object);

#endif