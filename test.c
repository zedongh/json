#include <assert.h>
#include <string.h>
#include "json_t.h"
#include "parser.h"

void test_parse_true() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "true");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_TRUE);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_false() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "false");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_FALSE);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_null() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "null");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_NULL);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_string() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "\"abc\"");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_STRING);
    assert(strcmp(json.str_val.buf, "abc") == 0);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_array() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "[]");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_ARRAY);
    assert(json.array_val.len == 0);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_object() {
    struct parse_context_t ctx;
    parse_context_init(&ctx, "{}");
    struct json_t json;
    assert(parse(&ctx, &json) == 0);
    assert(json.kind == JSON_OBJECT);
    assert(json.object_val.len == 0);
    json_finalize(&json);
    parse_context_finalize(&ctx);
}

void test_parse_misc() {
    char *broken[] = {
            "{",
            "[",
            "nu",
            "tru",
            "fal",
            "\"",
            "'",
            "true1",
            "false2",
            "null3",
            "\"\"\""
    };
    size_t len = sizeof(broken) / sizeof(char *);
    for (int i = 0; i < len; ++i) {
        struct parse_context_t ctx;
        parse_context_init(&ctx, broken[i]);
        struct json_t json;
        json_init(&json, JSON_UNKNOWN);
        assert(parse(&ctx, &json));
        json_finalize(&json);
        parse_context_finalize(&ctx);
    }
}

void test_parse_complex() {
    char *broken[] = {
            "{\"k1\": 1}",
            "[1, true, false]",
            " false ",
            " [ 1 ] ",
            " [{},{}, {}]",
            " [\"abc\", {}]",
            " {\"kx\": [], \"ky\": {}}",
    };
    size_t len = sizeof(broken) / sizeof(char *);
    for (int i = 0; i < len; ++i) {
        struct parse_context_t ctx;
        parse_context_init(&ctx, broken[i]);
        struct json_t json;
        json_init(&json, JSON_UNKNOWN);
        assert(parse(&ctx, &json) == 0);
        json_finalize(&json);
        parse_context_finalize(&ctx);
    }
}

void test_parse_idempotent() {
    char *broken[] = {
            "{\"k1\": 1}",
            "[1, true, false]",
            " false ",
            " [ 1 ] ",
            " [{},{}, {}]",
            " [\"abc\", {}]",
            " {\"kx\": [], \"ky\": {}}",
    };
    size_t len = sizeof(broken) / sizeof(char *);
    for (int i = 0; i < len; ++i) {
        struct parse_context_t ctx;
        parse_context_init(&ctx, broken[i]);
        struct json_t json;
        json_init(&json, JSON_UNKNOWN);
        assert(parse(&ctx, &json) == 0);
        struct json_string buf;
        json_string_init(&buf);
        json_stringify(&json, &buf);
        json_finalize(&json);

        struct parse_context_t ctx2;
        parse_context_init(&ctx2, buf.buf);
        struct json_t json2;
        assert(parse(&ctx2, &json2) == 0);
        struct json_string buf2;
        json_string_init(&buf2);
        json_stringify(&json2, &buf2);
        json_finalize(&json2);
        assert(buf.len == buf2.len);
        assert(strncmp(buf.buf, buf2.buf, buf.len) == 0);

        json_string_finalize(&buf2);
        parse_context_finalize(&ctx2);
        json_string_finalize(&buf);
        parse_context_finalize(&ctx);
    }
}

void test_parse_file() {
    char *files[] = {
            "samples/canada.json",
            "samples/citm_catalog.json",
            "samples/twitter.json",
    };
    size_t len = sizeof(files) / sizeof(char *);
    for (int i = 0; i < len; ++i) {
        struct parse_context_t ctx;
        assert(parse_context_init_from_file_path(&ctx, files[i]) == 0);
        struct json_t json;
        json_init(&json, JSON_UNKNOWN);
        assert(parse(&ctx, &json) == 0);
        json_finalize(&json);
        parse_context_finalize(&ctx);
    }
}

void test_hash_map() {
    struct json_object map;
    json_object_init(&map);
    assert(map.len == 0);
    struct json_string key;
    json_string_init(&key);
    json_string_append(&key, "abc");
    json_object_put(&map, key, json_number(123));
    assert(map.len == 1);
    struct json_t *value = json_object_get(&map, key);
    assert(value);
    assert(value->kind = JSON_NUMBER);
    assert(value->num_val == 123);
    json_object_put(&map, key, json_null);
    assert(map.len == 1);
    value = json_object_get(&map, key);
    assert(value);
    assert(value->kind = JSON_NULL);
    json_object_put(&map, key, json_false);
    assert(map.len == 1);
    value = json_object_get(&map, key);
    assert(value);
    assert(value->kind = JSON_FALSE);
    json_object_finalize(&map);
}

void test_linked_hash_map() {
    const char *keys[] = {
            "key1",
            "key2",
            "key3",
            "key4",
            "key5",
    };
    struct json_object map;
    json_object_init(&map);
    assert(map.len == 0);
    size_t len = sizeof(keys) / sizeof(char *);
    for (int i = 0; i < len; ++i) {
        struct json_string key;
        json_string_init(&key);
        json_string_append(&key, keys[i]);
        json_object_put(&map, key, json_number(i));
        assert(map.len == i + 1);
    }
    struct object_entry *entries = json_object_entries(&map);
    for (int i = 0; i < len; ++i) {
        assert(entries);
        assert(entries->value.kind == JSON_NUMBER);
        assert(entries->value.num_val == i);
        assert(strncmp(entries->key.buf, keys[i], strlen(keys[i])) == 0);
        entries = entries->link_next;
    }
    json_object_finalize(&map);
}

int main(int argc, char **argv) {
    test_parse_true();
    test_parse_false();
    test_parse_null();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_parse_misc();
    test_parse_complex();
    test_parse_file();
    test_hash_map();
    test_linked_hash_map();
    test_parse_idempotent();
    return 0;
}