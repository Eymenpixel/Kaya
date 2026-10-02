#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define JSON_MAX_DEPTH 64

typedef struct {
    const char *p;
    const char *err;
    int depth;
} parser;

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fputs("json: bellek yetersiz\n", stderr); exit(1); }
    return p;
}

static void *xrealloc(void *old, size_t n) {
    void *p = realloc(old, n ? n : 1);
    if (!p) { fputs("json: bellek yetersiz\n", stderr); exit(1); }
    return p;
}

static void skip_ws(parser *ps) {
    while (*ps->p && isspace((unsigned char)*ps->p)) ps->p++;
}

static json_value *parse_value(parser *ps);

static json_value *new_value(json_type t) {
    json_value *v = calloc(1, sizeof(json_value));
    if (!v) { fputs("json: bellek yetersiz\n", stderr); exit(1); }
    v->type = t;
    return v;
}

static char *parse_raw_string(parser *ps) {
    /* assumes *ps->p == '"' */
    ps->p++;
    size_t cap = 32, len = 0;
    char *buf = xmalloc(cap);
    while (*ps->p && *ps->p != '"') {
        char c = *ps->p++;
        if (c == '\\') {
            if (!*ps->p) break; /* metin '\' ile bitti: NUL'un ötesine okuma */
            char e = *ps->p++;
            switch (e) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'u': {
                    /* en fazla 4 hex hanesini atla, '?' yaz (tam unicode gerekmiyor) */
                    for (int i = 0; i < 4 && isxdigit((unsigned char)*ps->p); i++) ps->p++;
                    c = '?';
                    break;
                }
                default: c = e;
            }
        }
        if (len + 1 >= cap) { cap *= 2; buf = xrealloc(buf, cap); }
        buf[len++] = c;
    }
    if (*ps->p == '"') ps->p++;
    else if (!ps->err) ps->err = "unterminated string";
    buf[len] = '\0';
    return buf;
}

static json_value *parse_string(parser *ps) {
    json_value *v = new_value(JSON_STRING);
    v->u.string = parse_raw_string(ps);
    return v;
}

static json_value *parse_number(parser *ps) {
    const char *start = ps->p;
    const char *digits_from;
    if (*ps->p == '-' || *ps->p == '+') ps->p++;
    digits_from = ps->p;
    while (isdigit((unsigned char)*ps->p)) ps->p++;
    if (ps->p == digits_from) { ps->err = "invalid number"; return NULL; }
    if (*ps->p == '.') { ps->p++; while (isdigit((unsigned char)*ps->p)) ps->p++; }
    if (*ps->p == 'e' || *ps->p == 'E') {
        ps->p++;
        if (*ps->p == '-' || *ps->p == '+') ps->p++;
        while (isdigit((unsigned char)*ps->p)) ps->p++;
    }
    json_value *v = new_value(JSON_NUMBER);
    v->u.number = strtod(start, NULL);
    return v;
}

static json_value *parse_array(parser *ps) {
    if (++ps->depth > JSON_MAX_DEPTH) { ps->err = "nesting too deep"; return NULL; }
    json_value *v = new_value(JSON_ARRAY);
    ps->p++; /* [ */
    skip_ws(ps);
    size_t cap = 4;
    v->u.array.items = xmalloc(cap * sizeof(json_value *));
    v->u.array.count = 0;
    if (*ps->p == ']') { ps->p++; ps->depth--; return v; }
    int closed = 0;
    while (*ps->p) {
        skip_ws(ps);
        json_value *item = parse_value(ps);
        if (!item) { if (!ps->err) ps->err = "invalid array item"; break; }
        if (v->u.array.count == cap) {
            cap *= 2;
            v->u.array.items = xrealloc(v->u.array.items, cap * sizeof(json_value *));
        }
        v->u.array.items[v->u.array.count++] = item;
        if (ps->err) break;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; closed = 1; break; }
        ps->err = "expected ',' or ']' in array";
        break;
    }
    if (!closed && !ps->err) ps->err = "unterminated array";
    ps->depth--;
    return v;
}

static json_value *parse_object(parser *ps) {
    if (++ps->depth > JSON_MAX_DEPTH) { ps->err = "nesting too deep"; return NULL; }
    json_value *v = new_value(JSON_OBJECT);
    ps->p++; /* { */
    skip_ws(ps);
    size_t cap = 4;
    v->u.object.keys = xmalloc(cap * sizeof(char *));
    v->u.object.values = xmalloc(cap * sizeof(json_value *));
    v->u.object.count = 0;
    if (*ps->p == '}') { ps->p++; ps->depth--; return v; }
    int closed = 0;
    while (*ps->p) {
        skip_ws(ps);
        if (*ps->p != '"') { ps->err = "expected string key"; break; }
        char *key = parse_raw_string(ps);
        skip_ws(ps);
        if (*ps->p != ':') { if (!ps->err) ps->err = "expected ':'"; free(key); break; }
        ps->p++;
        skip_ws(ps);
        json_value *val = parse_value(ps);
        if (!val) { if (!ps->err) ps->err = "invalid object value"; free(key); break; }
        if (v->u.object.count == cap) {
            cap *= 2;
            v->u.object.keys = xrealloc(v->u.object.keys, cap * sizeof(char *));
            v->u.object.values = xrealloc(v->u.object.values, cap * sizeof(json_value *));
        }
        v->u.object.keys[v->u.object.count] = key;
        v->u.object.values[v->u.object.count] = val;
        v->u.object.count++;
        if (ps->err) break;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; closed = 1; break; }
        ps->err = "expected ',' or '}' in object";
        break;
    }
    if (!closed && !ps->err) ps->err = "unterminated object";
    ps->depth--;
    return v;
}

static json_value *parse_value(parser *ps) {
    skip_ws(ps);
    switch (*ps->p) {
        case '"': return parse_string(ps);
        case '{': return parse_object(ps);
        case '[': return parse_array(ps);
        case 't':
            if (strncmp(ps->p, "true", 4) == 0) { ps->p += 4; json_value *v = new_value(JSON_BOOL); v->u.boolean = 1; return v; }
            ps->err = "invalid token"; return NULL;
        case 'f':
            if (strncmp(ps->p, "false", 5) == 0) { ps->p += 5; json_value *v = new_value(JSON_BOOL); v->u.boolean = 0; return v; }
            ps->err = "invalid token"; return NULL;
        case 'n':
            if (strncmp(ps->p, "null", 4) == 0) { ps->p += 4; return new_value(JSON_NULL); }
            ps->err = "invalid token"; return NULL;
        default:
            if (*ps->p == '-' || isdigit((unsigned char)*ps->p)) return parse_number(ps);
            ps->err = "unexpected character";
            return NULL;
    }
}

json_value *json_parse(const char *text, const char **err) {
    parser ps = { text, NULL, 0 };
    json_value *v = parse_value(&ps);
    if (!ps.err) {
        skip_ws(&ps);
        if (*ps.p) ps.err = "trailing data after JSON value";
    }
    if (ps.err) {
        if (err) *err = ps.err;
        if (v) json_free(v);
        return NULL;
    }
    if (err) *err = NULL;
    return v;
}

void json_free(json_value *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->u.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->u.array.count; i++) json_free(v->u.array.items[i]);
            free(v->u.array.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->u.object.count; i++) {
                free(v->u.object.keys[i]);
                json_free(v->u.object.values[i]);
            }
            free(v->u.object.keys);
            free(v->u.object.values);
            break;
        default:
            break;
    }
    free(v);
}

json_value *json_object_get(const json_value *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < obj->u.object.count; i++) {
        if (strcmp(obj->u.object.keys[i], key) == 0) return obj->u.object.values[i];
    }
    return NULL;
}

const char *json_get_string(const json_value *obj, const char *key, const char *def) {
    json_value *v = json_object_get(obj, key);
    if (v && v->type == JSON_STRING) return v->u.string;
    return def;
}

double json_get_number(const json_value *obj, const char *key, double def) {
    json_value *v = json_object_get(obj, key);
    if (v && v->type == JSON_NUMBER) return v->u.number;
    return def;
}

const char **json_get_string_array(const json_value *obj, const char *key, size_t *count) {
    json_value *v = json_object_get(obj, key);
    if (!v || v->type != JSON_ARRAY) { if (count) *count = 0; return NULL; }
    const char **out = xmalloc(v->u.array.count * sizeof(char *));
    size_t n = 0;
    for (size_t i = 0; i < v->u.array.count; i++) {
        if (v->u.array.items[i]->type == JSON_STRING) out[n++] = v->u.array.items[i]->u.string;
    }
    if (count) *count = n;
    return out;
}
