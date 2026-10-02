#ifndef KAYA_JSON_H
#define KAYA_JSON_H

#include <stddef.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct json_value {
    json_type type;
    union {
        int boolean;
        double number;
        char *string;
        struct { struct json_value **items; size_t count; } array;
        struct { char **keys; struct json_value **values; size_t count; } object;
    } u;
} json_value;

json_value *json_parse(const char *text, const char **err);
void json_free(json_value *v);
json_value *json_object_get(const json_value *obj, const char *key);
const char *json_get_string(const json_value *obj, const char *key, const char *def);
double json_get_number(const json_value *obj, const char *key, double def);
const char **json_get_string_array(const json_value *obj, const char *key, size_t *count);

#endif
