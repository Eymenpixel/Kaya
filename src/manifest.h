#ifndef KAYA_MANIFEST_H
#define KAYA_MANIFEST_H

typedef struct {
    char name[64];
    char display_name[128];
    char version[32];
    char author[128];
    char description[256];
    int package_format;
    char runtime[32];    /* "native", "python", "sh", "sb3", "wasm", ... default "native" */
    char language[32];
    char entry[128];
    char icon[128];
    char category[64];

    char **dependencies;
    int dep_count;
    char **permissions;
    int perm_count;
    /* Gerekli plugin ADLARI (indirme adresi değil). Yalnızca resmi depodan kurulur. */
    char **plugins;
    int plugin_count;
} manifest_t;

/* Parses manifest JSON text into *out. Returns 0 on success, -1 on error
 * (err set to a short static message). */
int manifest_parse(const char *json_text, manifest_t *out, const char **err);
void manifest_free(manifest_t *m);
void manifest_print(const manifest_t *m);

#endif
