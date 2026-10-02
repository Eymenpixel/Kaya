#define _POSIX_C_SOURCE 200809L
#include "manifest.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void copy_str_array(const json_value *root, const char *key, char ***dst, int *count) {
    size_t n = 0;
    const char **src = json_get_string_array(root, key, &n);
    *count = (int)n;
    *dst = NULL;
    if (n > 0) {
        *dst = malloc(n * sizeof(char *));
        if (!*dst) { *count = 0; free(src); return; }
        for (size_t i = 0; i < n; i++) (*dst)[i] = strdup(src[i]);
    }
    free(src);
}

int manifest_parse(const char *json_text, manifest_t *out, const char **err) {
    memset(out, 0, sizeof(*out));
    const char *jerr = NULL;
    json_value *root = json_parse(json_text, &jerr);
    if (!root) { if (err) *err = jerr ? jerr : "invalid JSON"; return -1; }
    if (root->type != JSON_OBJECT) { if (err) *err = "manifest root must be an object"; json_free(root); return -1; }

    const char *name = json_get_string(root, "name", NULL);
    if (!name || !*name) { if (err) *err = "manifest missing required field: name"; json_free(root); return -1; }
    const char *entry = json_get_string(root, "entry", NULL);
    if (!entry || !*entry) { if (err) *err = "manifest missing required field: entry"; json_free(root); return -1; }

    strncpy(out->name, name, sizeof(out->name) - 1);
    strncpy(out->entry, entry, sizeof(out->entry) - 1);
    strncpy(out->display_name, json_get_string(root, "displayName", name), sizeof(out->display_name) - 1);
    strncpy(out->version, json_get_string(root, "version", "0.0.0"), sizeof(out->version) - 1);
    strncpy(out->author, json_get_string(root, "author", ""), sizeof(out->author) - 1);
    strncpy(out->description, json_get_string(root, "description", ""), sizeof(out->description) - 1);
    strncpy(out->runtime, json_get_string(root, "runtime", "native"), sizeof(out->runtime) - 1);
    strncpy(out->language, json_get_string(root, "language", ""), sizeof(out->language) - 1);
    strncpy(out->icon, json_get_string(root, "icon", ""), sizeof(out->icon) - 1);
    strncpy(out->category, json_get_string(root, "category", ""), sizeof(out->category) - 1);
    out->package_format = (int)json_get_number(root, "packageFormat", 1);

    copy_str_array(root, "dependencies", &out->dependencies, &out->dep_count);
    copy_str_array(root, "permissions", &out->permissions, &out->perm_count);
    copy_str_array(root, "plugins", &out->plugins, &out->plugin_count);

    json_free(root);
    if (err) *err = NULL;
    return 0;
}

void manifest_free(manifest_t *m) {
    for (int i = 0; i < m->dep_count; i++) free(m->dependencies[i]);
    free(m->dependencies);
    m->dependencies = NULL; m->dep_count = 0;
    for (int i = 0; i < m->perm_count; i++) free(m->permissions[i]);
    free(m->permissions);
    m->permissions = NULL; m->perm_count = 0;
    for (int i = 0; i < m->plugin_count; i++) free(m->plugins[i]);
    free(m->plugins);
    m->plugins = NULL; m->plugin_count = 0;
}

void manifest_print(const manifest_t *m) {
    printf("  name        : %s\n", m->name);
    printf("  displayName : %s\n", m->display_name);
    printf("  version     : %s\n", m->version);
    if (*m->author) printf("  author      : %s\n", m->author);
    if (*m->description) printf("  description : %s\n", m->description);
    printf("  runtime     : %s\n", m->runtime);
    if (*m->language) printf("  language    : %s\n", m->language);
    printf("  entry       : %s\n", m->entry);
    if (*m->category) printf("  category    : %s\n", m->category);
    if (m->dep_count > 0) {
        printf("  dependencies:\n");
        for (int i = 0; i < m->dep_count; i++) printf("    - %s\n", m->dependencies[i]);
    }
    if (m->plugin_count > 0) {
        printf("  plugins     :\n");
        for (int i = 0; i < m->plugin_count; i++) printf("    - %s\n", m->plugins[i]);
    }
    if (m->perm_count > 0) {
        printf("  permissions :\n");
        for (int i = 0; i < m->perm_count; i++) printf("    - %s\n", m->permissions[i]);
    }
}
