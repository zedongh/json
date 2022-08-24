//
// Created by LAMBDA on 2022/8/23.
//

#include <stdlib.h>
#include <string.h>

#include "json_t.h"
#include "util.h"

#define DEFAULT_CAPACITY 16
#define LOAD_FACTOR 0.75

unsigned long djb2_string_hash(struct json_string str) {
    unsigned long hash = 5381;
    for (int i = 0; i < str.len; ++i) {
        hash = ((hash << 5) + hash) + str.buf[i]; // hash * 33 + c
    }
    return hash;
}

void ensure_json_object_capacity(struct json_object *object, size_t more);

void json_object_init(struct json_object *object) {
    object->cap = DEFAULT_CAPACITY;
    object->slots = (struct object_entry **) calloc(object->cap, sizeof(struct object_entry *));
    object->len = 0;
    object->head = NULL;
    object->tail = NULL;
}

void json_object_finalize(struct json_object *object) {
    struct object_entry *current = object->head;
    while (current) {
        struct object_entry *backup = current->link_next;
        json_string_finalize(&current->key);
        json_finalize(&current->value);
        free(current);
        current = backup;
    }
    object->len = 0;
    free(object->slots);
    object->cap = 0;
    object->head = NULL;
    object->tail = NULL;
}

void _insert_into_slots(struct object_entry **slots, size_t slot_len, struct object_entry *entry) {
    size_t slot_at = djb2_string_hash(entry->key) % slot_len;
    struct object_entry *current = slots[slot_at];
    if (current == NULL) {
        slots[slot_at] = entry;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = entry;
    }
}

void json_object_put(struct json_object *object, struct json_string key, struct json_t value) {
    struct json_t *old_value = json_object_get(object, key);
    if (old_value != NULL) {
        json_finalize(old_value);
        (*old_value) = value;
        return;
    }
    ensure_json_object_capacity(object, 1);
    struct object_entry *entry = (struct object_entry *) calloc(1, sizeof(struct object_entry));
    entry->key = key;
    entry->value = value;
    _insert_into_slots(object->slots, object->cap, entry);
    object->len++;
    if (object->len == 1) {
        object->head = entry;
        object->tail = entry;
    } else {
        object->tail->link_next = entry;
        entry->link_prev = object->tail;
        object->tail = entry;
    }
}

struct object_entry *json_object_entries(struct json_object *object) {
    return object->head;
}

struct json_t *json_object_get(struct json_object *object, struct json_string key) {
    size_t slot_at = djb2_string_hash(key) % object->cap;
    struct object_entry *current = object->slots[slot_at];
    while (current) {
        if (current->key.len == key.len && strncmp(current->key.buf, key.buf, key.len) == 0) {
            return &current->value;
        }
        current = current->next;
    }
    return NULL;
}

void ensure_json_object_capacity(struct json_object *object, size_t more) {
    if (object->len + more < (size_t) ((double) object->cap * LOAD_FACTOR)) {
        return;
    }
    size_t new_cap = next_bin_exp(object->cap + object->cap / 2);
    struct object_entry **new_slots = (struct object_entry **) calloc(new_cap, sizeof (struct object_entry *));
    struct object_entry *current = object->head;
    while (current) {
        current->next = NULL; // avoid copy cycle
        _insert_into_slots(new_slots, new_cap, current);
        current = current->link_next;
    }
    object->slots = new_slots;
    object->cap = new_cap;
}
