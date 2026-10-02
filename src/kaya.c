#define _POSIX_C_SOURCE 200809L
/* kaya - package manager for .kay packages (BlackBit-style)
 *
 * Usage:
 *   kaya --install <isim>                   repodan paket kurar
 *   kaya --install -file <dosyayolu/xyz.kay>        yerel .kay dosyasını kurar
 *   kaya --install -plugin <isim>           resmi depodan plugin kurar
 *   kaya --remove <isim>
 *   kaya --remove -plugin <isim>
 *   kaya --upgrade
 *   kaya --relook
 *   kaya list
 *   kaya plugins
 *   kaya info <name>
 *   kaya run <name> [-- args...]
 *
 * Tüm komutlar --root DIR kabul eder (varsayılan: ./kaya_root). Düzen:
 *   <root>/apps/<name>/manifest.json
 *   <root>/apps/<name>/<...paket dosyaları...>
 *   <root>/plugins/<name>/plugin.json
 *   <root>/plugins/<name>/<...plugin dosyaları...>
 *
 * Resmi plugin sistemi:
 *   - Pluginler YALNIZCA Eymenpixel/Kaya deposunun Plugins/<isim>/ klasöründen
 *     kurulur. Başka repo, fork, URL ya da yerel .kyp dosyası kabul edilmez.
 *   - Paket manifesti plugin'i sadece ADIYLA isteyebilir ("plugins": ["scratch"]).
 *   - Eksik plugin, paket kurulurken resmi depodan otomatik kurulur.
 *   - Her plugin bir plugin.json taşır (name, version, description, permissions).
 *     Bilinen izinler: run_process, filesystem, network.
 *
 * Plugin çalıştırma kuralı: plugin dizininin kökünde "run" adlı çalıştırılabilir
 * bir dosya varsa kaya onu "<plugins>/<name>/run <entry_abspath> [args...]"
 * şeklinde (shell yok, doğrudan exec) çağırır. Bunun için pluginin
 * "run_process" izni olmalıdır.
 *   sb3  -> "scratch" plugin'i (TurboWarp). Kurulu değilse PyStage'e düşer.
 *   wasm / c / cpp -> aynı adlı plugin (zorunlu).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "tar.h"
#include "manifest.h"
#include "json.h"

#define DEFAULT_ROOT "./kaya_root"
#define KAYA_VERSION "1.0-beta.3"

/* Depo düzeni: her paket/plugin kendi klasöründe, dosya adı serbest
 * (Packages/clock/whatever.kay). GitHub "contents" API'siyle klasör listelenir. */
#define PACKAGES_API_BASE "https://api.github.com/repos/Eymenpixel/Kaya/contents/Packages"
#define PLUGINS_API_BASE  "https://api.github.com/repos/Eymenpixel/Kaya/contents/Plugins"
#define RELEASES_API      "https://api.github.com/repos/Eymenpixel/Kaya/releases/latest"
#define INDEX_URL         "https://raw.githubusercontent.com/Eymenpixel/Kaya/main/index.json"
#define RELEASES_URL      "https://github.com/Eymenpixel/Kaya/releases/latest"
/* İndirme adresleri bu önekle başlamak ZORUNDA (resmi depo dışı reddedilir). */
#define OFFICIAL_RAW_PREFIX "https://raw.githubusercontent.com/Eymenpixel/Kaya/"

static char root_dir[512];
static char browser_override[128] = "";

static void apps_dir(char *out, size_t n) { snprintf(out, n, "%s/apps", root_dir); }
static void app_dir(const char *name, char *out, size_t n) { snprintf(out, n, "%s/apps/%s", root_dir, name); }
static void manifest_path(const char *name, char *out, size_t n) { snprintf(out, n, "%s/apps/%s/manifest.json", root_dir, name); }
static void plugins_dir(char *out, size_t n) { snprintf(out, n, "%s/plugins", root_dir); }
static void plugin_dir(const char *name, char *out, size_t n) { snprintf(out, n, "%s/plugins/%s", root_dir, name); }

/* --- doğrulama yardımcıları -------------------------------------------- */

/* Dizin adı olacak her string (paket adı, plugin adı, bağımlılık adı) buradan
 * geçer: yalnızca harf/rakam/'-'/'_'/'.', başta '.' yok, en fazla 63 karakter.
 * Böylece "..", "a/b", URL ("https://...") gibi şeyler baştan elenir. */
static int is_safe_component(const char *s) {
    if (!s || !*s || strlen(s) >= 64) return 0;
    if (s[0] == '.') return 0;
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (!(isalnum(c) || c == '-' || c == '_' || c == '.')) return 0;
    }
    return 1;
}

/* Manifestteki "entry": göreli yol, '/' ile başlamaz, ".." içermez. */
static int is_safe_rel_path(const char *s) {
    if (!s || !*s) return 0;
    if (s[0] == '/') return 0;
    if (strstr(s, "..")) return 0;
    return 1;
}

/* --- alt süreç yardımcıları -------------------------------------------- */

static int run_argv(char *const argv[], const char *log_path) {
    pid_t pid = fork();
    if (pid < 0) { perror("kaya: fork"); return -1; }
    if (pid == 0) {
        if (log_path) {
            int fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        execvp(argv[0], argv);
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

static int download_file(const char *url, const char *dest_path) {
    /* Yalnızca https (yönlendirmeler dahil), boyut sınırlı. -A: GitHub API
     * User-Agent'sız istekleri 403'ler. */
    char *argv[] = {
        "curl", "-fsSL",
        "--proto", "=https", "--proto-redir", "=https",
        "--max-filesize", "52428800",
        "-A", "kaya-package-manager",
        "-o", (char *)dest_path, (char *)url, NULL
    };
    int rc = run_argv(argv, NULL);
    if (rc != 0) {
        fprintf(stderr, "kaya: indirme başarısız: %s (curl kurulu mu?)\n", url);
        return -1;
    }
    return 0;
}

/* Tahmin edilemez, 0700 izinli geçici dizin (symlink saldırısına karşı). */
static int make_tmpdir(char *out, size_t n) {
    const char *base = getenv("TMPDIR");
    if (!base || !*base) base = "/tmp";
    snprintf(out, n, "%s/kaya-XXXXXX", base);
    if (!mkdtemp(out)) { perror("kaya: mkdtemp"); return -1; }
    return 0;
}

/* --- dosya yardımcıları ------------------------------------------------ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)size, f);
    buf[got] = '\0';
    fclose(f);
    return buf;
}

static int rm_recursive(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0; /* lstat: symlink'i takip etme */
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;
        struct dirent *ent;
        while ((ent = readdir(d))) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char child[1024];
            snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
            rm_recursive(child);
        }
        closedir(d);
        rmdir(path);
    } else {
        unlink(path);
    }
    return 0;
}

static void mkdirs(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = '\0'; mkdir(tmp, 0755); *p = '/'; }
    }
    mkdir(tmp, 0755);
}

/* <parent>/.staging-XXXXXX (aynı dosya sistemi, rename için) */
static int make_staging(const char *parent, char *out, size_t n) {
    snprintf(out, n, "%s/.staging-XXXXXX", parent);
    if (!mkdtemp(out)) { perror("kaya: mkdtemp"); return -1; }
    chmod(out, 0755);
    return 0;
}

/* Eskiyi yedeğe al, yeniyi yerine koy; olmazsa eskiyi geri getir.
 * Çıkarma başarısız olursa kurulu sürüm asla kaybolmaz. */
static int swap_dir(const char *staging, const char *final_path, const char *backup_path) {
    rm_recursive(backup_path);
    int had_old = (rename(final_path, backup_path) == 0);
    if (rename(staging, final_path) != 0) {
        perror("kaya: rename");
        if (had_old) rename(backup_path, final_path);
        return -1;
    }
    rm_recursive(backup_path);
    return 0;
}

/* Depo klasörünü API ile listeler, `ext` ile biten ilk dosyanın download_url'ini
 * döndürür. URL'in resmi depodan ve beklenen klasörden (`required_infix`,
 * örn. "/Plugins/scratch/") geldiği doğrulanır; aksi halde NULL. */
static char *github_dir_find_download_url(const char *api_url, const char *ext, const char *required_infix) {
    char tdir[600];
    if (make_tmpdir(tdir, sizeof(tdir)) != 0) return NULL;
    char tmp[700];
    snprintf(tmp, sizeof(tmp), "%s/listing.json", tdir);
    if (download_file(api_url, tmp) != 0) { rm_recursive(tdir); return NULL; }
    char *text = read_file(tmp);
    rm_recursive(tdir);
    if (!text) return NULL;
    const char *jerr;
    json_value *root = json_parse(text, &jerr);
    free(text);
    if (!root) {
        fprintf(stderr, "kaya: depo yanıtı ayrıştırılamadı: %s\n", jerr ? jerr : "?");
        return NULL;
    }
    if (root->type != JSON_ARRAY) { json_free(root); return NULL; }
    char *result = NULL;
    size_t elen = strlen(ext);
    for (size_t i = 0; i < root->u.array.count; i++) {
        json_value *item = root->u.array.items[i];
        const char *name = json_get_string(item, "name", NULL);
        const char *type = json_get_string(item, "type", "");
        if (!name || strcmp(type, "file") != 0) continue;
        size_t nlen = strlen(name);
        if (nlen > elen && strcmp(name + nlen - elen, ext) == 0) {
            const char *durl = json_get_string(item, "download_url", NULL);
            if (!durl) continue;
            if (strncmp(durl, OFFICIAL_RAW_PREFIX, strlen(OFFICIAL_RAW_PREFIX)) != 0 ||
                !strstr(durl, required_infix) || strstr(durl, "..")) {
                fprintf(stderr, "kaya: resmi depo dışı indirme adresi reddedildi: %s\n", durl);
                continue;
            }
            result = strdup(durl);
            break;
        }
    }
    json_free(root);
    return result;
}

static int load_installed_manifest(const char *name, manifest_t *m) {
    if (!is_safe_component(name)) return -1;
    char path[600];
    manifest_path(name, path, sizeof(path));
    char *text = read_file(path);
    if (!text) return -1;
    const char *err;
    int rc = manifest_parse(text, m, &err);
    free(text);
    return rc;
}

/* --- sürüm / bağımlılık ------------------------------------------------ */

static int version_cmp(const char *a, const char *b) {
    while (*a || *b) {
        long na = 0, nb = 0;
        while (isdigit((unsigned char)*a)) { if (na < 100000000) na = na * 10 + (*a - '0'); a++; }
        while (isdigit((unsigned char)*b)) { if (nb < 100000000) nb = nb * 10 + (*b - '0'); b++; }
        if (na != nb) return na < nb ? -1 : 1;
        while (*a && *a != '.') a++; /* "1.0-beta" gibi rakam olmayan kısımları atla */
        while (*b && *b != '.') b++;
        if (*a == '.') a++;
        if (*b == '.') b++;
    }
    return 0;
}

static void parse_dep(const char *spec, char *name, size_t nlen, char *op, size_t olen, char *ver, size_t vlen) {
    const char *ops[] = { ">=", "<=", "==", ">", "<" };
    for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
        const char *pos = strstr(spec, ops[i]);
        if (pos) {
            size_t nl = pos - spec;
            snprintf(name, nlen, "%.*s", (int)nl, spec);
            snprintf(op, olen, "%s", ops[i]);
            snprintf(ver, vlen, "%s", pos + strlen(ops[i]));
            return;
        }
    }
    snprintf(name, nlen, "%s", spec);
    op[0] = '\0';
    ver[0] = '\0';
}

static int dep_satisfied(const char *installed_version, const char *op, const char *required_version) {
    int c = version_cmp(installed_version, required_version);
    if (strcmp(op, ">=") == 0) return c >= 0;
    if (strcmp(op, "<=") == 0) return c <= 0;
    if (strcmp(op, "==") == 0) return c == 0;
    if (strcmp(op, ">") == 0) return c > 0;
    if (strcmp(op, "<") == 0) return c < 0;
    return 1;
}

static int is_installed(const char *name) {
    if (!is_safe_component(name)) return 0;
    char path[600];
    manifest_path(name, path, sizeof(path));
    struct stat st;
    return stat(path, &st) == 0;
}

static int is_plugin_installed(const char *name) {
    if (!is_safe_component(name)) return 0;
    char p[600];
    plugin_dir(name, p, sizeof(p));
    struct stat st;
    return lstat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

/* --- plugin'ler (plugin.json, izinler, resmi depo) --------------------- */

typedef struct {
    char name[64];
    char version[32];
    char description[256];
    char **perms;
    int perm_count;
} plugin_info;

static const char *KNOWN_PLUGIN_PERMS[] = { "run_process", "filesystem", "network", NULL };

static int is_known_perm(const char *p) {
    for (int i = 0; KNOWN_PLUGIN_PERMS[i]; i++)
        if (strcmp(p, KNOWN_PLUGIN_PERMS[i]) == 0) return 1;
    return 0;
}

static void plugin_info_free(plugin_info *pi) {
    for (int i = 0; i < pi->perm_count; i++) free(pi->perms[i]);
    free(pi->perms);
    pi->perms = NULL;
    pi->perm_count = 0;
}

static int plugin_info_parse(const char *text, plugin_info *pi, const char **err) {
    memset(pi, 0, sizeof(*pi));
    const char *jerr = NULL;
    json_value *root = json_parse(text, &jerr);
    if (!root) { *err = jerr ? jerr : "geçersiz JSON"; return -1; }
    if (root->type != JSON_OBJECT) { *err = "plugin.json bir nesne olmalı"; json_free(root); return -1; }
    const char *name = json_get_string(root, "name", NULL);
    if (!name || !is_safe_component(name)) { *err = "plugin.json: 'name' eksik ya da geçersiz"; json_free(root); return -1; }
    snprintf(pi->name, sizeof(pi->name), "%s", name);
    snprintf(pi->version, sizeof(pi->version), "%s", json_get_string(root, "version", "0.0.0"));
    snprintf(pi->description, sizeof(pi->description), "%s", json_get_string(root, "description", ""));

    size_t n = 0;
    const char **perms = json_get_string_array(root, "permissions", &n);
    for (size_t i = 0; i < n; i++) {
        if (!is_known_perm(perms[i])) {
            *err = "plugin.json: bilinmeyen izin";
            free(perms);
            json_free(root);
            return -1;
        }
    }
    if (n > 0) {
        pi->perms = malloc(n * sizeof(char *));
        if (pi->perms) {
            for (size_t i = 0; i < n; i++) pi->perms[i] = strdup(perms[i]);
            pi->perm_count = (int)n;
        }
    }
    free(perms);
    json_free(root);
    return 0;
}

static int load_plugin_info(const char *name, plugin_info *pi) {
    if (!is_safe_component(name)) return -1;
    char path[700];
    snprintf(path, sizeof(path), "%s/plugins/%s/plugin.json", root_dir, name);
    char *text = read_file(path);
    if (!text) return -1;
    const char *err;
    int rc = plugin_info_parse(text, pi, &err);
    free(text);
    return rc;
}

static int plugin_has_perm(const plugin_info *pi, const char *perm) {
    for (int i = 0; i < pi->perm_count; i++)
        if (strcmp(pi->perms[i], perm) == 0) return 1;
    return 0;
}

/* İndirilmiş .kyp'yi kurar. Yalnızca resmi depodan indirilmiş dosyalar için
 * çağrılır (yerel .kyp kurulumu kaldırıldı). plugin.json zorunlu ve adı
 * istenen plugin adıyla aynı olmalı. */
static int install_plugin_archive(const char *kyp_file, const char *expected_name) {
    unsigned long msize;
    unsigned char *text = tar_read_entry(kyp_file, "plugin.json", &msize);
    if (!text) {
        fprintf(stderr, "kaya: plugin içinde plugin.json bulunamadı\n");
        return 1;
    }
    plugin_info pi;
    const char *err;
    int rc = plugin_info_parse((char *)text, &pi, &err);
    free(text);
    if (rc != 0) {
        fprintf(stderr, "kaya: plugin.json geçersiz: %s\n", err);
        return 1;
    }
    if (strcmp(pi.name, expected_name) != 0) {
        fprintf(stderr, "kaya: plugin.json adı ('%s') istenen plugin ('%s') ile uyuşmuyor\n", pi.name, expected_name);
        plugin_info_free(&pi);
        return 1;
    }

    char pdirs[600];
    plugins_dir(pdirs, sizeof(pdirs));
    mkdirs(pdirs);
    char staging[700];
    if (make_staging(pdirs, staging, sizeof(staging)) != 0) { plugin_info_free(&pi); return 1; }
    if (tar_extract_all(kyp_file, staging) != 0) {
        fprintf(stderr, "kaya: plugin açılamadı (bozuk ya da güvensiz arşiv)\n");
        rm_recursive(staging);
        plugin_info_free(&pi);
        return 1;
    }
    char runp[800];
    snprintf(runp, sizeof(runp), "%s/run", staging);
    struct stat st;
    if (lstat(runp, &st) == 0 && S_ISREG(st.st_mode)) chmod(runp, 0755);

    char pdir[600], backup[700];
    plugin_dir(expected_name, pdir, sizeof(pdir));
    snprintf(backup, sizeof(backup), "%s/.%s.old", pdirs, expected_name);
    if (swap_dir(staging, pdir, backup) != 0) {
        rm_recursive(staging);
        plugin_info_free(&pi);
        return 1;
    }

    printf("Plugin kuruldu: %s v%s\n", pi.name, pi.version);
    if (pi.perm_count > 0) {
        printf("  izinler:");
        for (int i = 0; i < pi.perm_count; i++) printf(" %s", pi.perms[i]);
        printf("\n");
    }
    plugin_info_free(&pi);
    return 0;
}

static int cmd_install_plugin_repo(const char *name) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz plugin adı: %s (yalnızca resmi depodaki plugin adı verilebilir)\n", name);
        return 1;
    }
    char api_url[700];
    snprintf(api_url, sizeof(api_url), PLUGINS_API_BASE "/%s", name);
    char infix[128];
    snprintf(infix, sizeof(infix), "/Plugins/%s/", name);
    printf("Aranıyor: Plugins/%s (resmi depo)\n", name);
    char *download_url = github_dir_find_download_url(api_url, ".kyp", infix);
    if (!download_url) {
        fprintf(stderr, "kaya: plugin '%s' resmi depoda bulunamadı (Plugins/%s içinde .kyp dosyası yok)\n", name, name);
        return 1;
    }
    char tdir[600];
    if (make_tmpdir(tdir, sizeof(tdir)) != 0) { free(download_url); return 1; }
    char tmp[700];
    snprintf(tmp, sizeof(tmp), "%s/plugin.kyp", tdir);
    printf("İndiriliyor: %s\n", download_url);
    int rc;
    if (download_file(download_url, tmp) != 0) rc = 1;
    else rc = install_plugin_archive(tmp, name);
    free(download_url);
    rm_recursive(tdir);
    return rc;
}

static int cmd_remove_plugin(const char *name) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz plugin adı: %s\n", name);
        return 1;
    }
    if (!is_plugin_installed(name)) {
        fprintf(stderr, "kaya: plugin '%s' kurulu değil\n", name);
        return 1;
    }
    char pdir[600];
    plugin_dir(name, pdir, sizeof(pdir));
    rm_recursive(pdir);
    printf("Plugin kaldırıldı: %s\n", name);
    return 0;
}

static int cmd_list_plugins(void) {
    char pdirs[600];
    plugins_dir(pdirs, sizeof(pdirs));
    DIR *d = opendir(pdirs);
    if (!d) { printf("Kurulu plugin yok.\n"); return 0; }
    struct dirent *ent;
    int count = 0;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] == '.') continue;
        plugin_info pi;
        if (load_plugin_info(ent->d_name, &pi) != 0) continue;
        printf("%-16s %-10s %s\n", pi.name, pi.version, pi.description);
        if (pi.perm_count > 0) {
            printf("  izinler:");
            for (int i = 0; i < pi.perm_count; i++) printf(" %s", pi.perms[i]);
            printf("\n");
        }
        plugin_info_free(&pi);
        count++;
    }
    closedir(d);
    if (count == 0) printf("Kurulu plugin yok.\n");
    return 0;
}

/* Paketin ihtiyaç duyduğu pluginleri (yalnızca ADLARI) resmi depodan kurar.
 *   manifest "plugins": zorunlu
 *   runtime "sb3"      : "scratch" isteğe bağlı (yoksa PyStage'e düşer)
 *   runtime "wasm"     : "wasm" zorunlu */
static int ensure_one_plugin(const char *name, int hard) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz plugin adı: %s (URL/yol verilemez, yalnızca plugin adı)\n", name);
        return 1;
    }
    if (is_plugin_installed(name)) return 0;
    printf("Gerekli plugin eksik, resmi depodan kuruluyor: %s\n", name);
    if (cmd_install_plugin_repo(name) == 0) return 0;
    if (hard) {
        fprintf(stderr, "kaya: gerekli plugin '%s' kurulamadı\n", name);
        return 1;
    }
    fprintf(stderr, "kaya: uyarı: isteğe bağlı plugin '%s' kurulamadı, yedek yöntem kullanılacak\n", name);
    return 0;
}

/* Yalnızca plugin ile çalışan runtime'lar: plugin adı = runtime adı. */
static int plugin_only_runtime(const char *rt) {
    return strcmp(rt, "wasm") == 0 || strcmp(rt, "c") == 0 || strcmp(rt, "cpp") == 0;
}

static int ensure_plugins(const manifest_t *m) {
    for (int i = 0; i < m->plugin_count; i++)
        if (ensure_one_plugin(m->plugins[i], 1) != 0) return 1;
    if (strcmp(m->runtime, "sb3") == 0) return ensure_one_plugin("scratch", 0);
    if (plugin_only_runtime(m->runtime)) return ensure_one_plugin(m->runtime, 1);
    return 0;
}

/* --- paket kur / kaldır ------------------------------------------------- */

static int cmd_install(const char *kay_file) {
    unsigned long msize;
    unsigned char *manifest_text = tar_read_entry(kay_file, "manifest.json", &msize);
    if (!manifest_text) {
        fprintf(stderr, "kaya: %s içinde manifest.json bulunamadı (ya da dosya bozuk)\n", kay_file);
        return 1;
    }
    manifest_t m;
    const char *err;
    if (manifest_parse((char *)manifest_text, &m, &err) != 0) {
        fprintf(stderr, "kaya: manifest.json geçersiz: %s\n", err);
        free(manifest_text);
        return 1;
    }
    free(manifest_text);

    if (!is_safe_component(m.name)) {
        fprintf(stderr, "kaya: manifestteki 'name' alanı geçersiz: %s\n", m.name);
        manifest_free(&m);
        return 1;
    }
    if (!is_safe_rel_path(m.entry)) {
        fprintf(stderr, "kaya: manifestteki 'entry' alanı geçersiz: %s\n", m.entry);
        manifest_free(&m);
        return 1;
    }

    printf("Kuruluyor: %s (%s) v%s\n", m.display_name, m.name, m.version);

    for (int i = 0; i < m.dep_count; i++) {
        char dname[64], dop[4], dver[32];
        parse_dep(m.dependencies[i], dname, sizeof(dname), dop, sizeof(dop), dver, sizeof(dver));
        manifest_t dep_m;
        if (load_installed_manifest(dname, &dep_m) != 0) {
            fprintf(stderr, "kaya: bağımlılık eksik: %s (gerekli: %s)\n", dname, m.dependencies[i]);
            manifest_free(&m);
            return 1;
        }
        int ok = dep_satisfied(dep_m.version, dop, dver);
        char have[32];
        snprintf(have, sizeof(have), "%s", dep_m.version);
        manifest_free(&dep_m);
        if (!ok) {
            fprintf(stderr, "kaya: bağımlılık uyuşmuyor: %s (gerekli: %s, kurulu: %s)\n",
                    dname, m.dependencies[i], have);
            manifest_free(&m);
            return 1;
        }
    }

    /* Eksik pluginleri resmi depodan kur (dosya sistemine dokunmadan önce). */
    if (ensure_plugins(&m) != 0) {
        manifest_free(&m);
        return 1;
    }

    char apps[600];
    apps_dir(apps, sizeof(apps));
    mkdirs(apps);
    char staging[700];
    if (make_staging(apps, staging, sizeof(staging)) != 0) { manifest_free(&m); return 1; }
    if (tar_extract_all(kay_file, staging) != 0) {
        fprintf(stderr, "kaya: %s açılamadı (eski sürüm korundu)\n", kay_file);
        rm_recursive(staging);
        manifest_free(&m);
        return 1;
    }

    if (strcmp(m.runtime, "native") == 0) {
        char entry_path[900];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", staging, m.entry);
        chmod(entry_path, 0755);
    }

    char adir[600], backup[700];
    app_dir(m.name, adir, sizeof(adir));
    snprintf(backup, sizeof(backup), "%s/.%s.old", apps, m.name);
    if (swap_dir(staging, adir, backup) != 0) {
        rm_recursive(staging);
        manifest_free(&m);
        return 1;
    }

    printf("Kurulum tamamlandı: %s/%s\n", apps, m.name);
    manifest_free(&m);
    return 0;
}

static int cmd_install_repo(const char *name) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz paket adı: %s\n", name);
        return 1;
    }
    char api_url[700];
    snprintf(api_url, sizeof(api_url), PACKAGES_API_BASE "/%s", name);
    char infix[128];
    snprintf(infix, sizeof(infix), "/Packages/%s/", name);
    printf("Aranıyor: Packages/%s\n", name);
    char *download_url = github_dir_find_download_url(api_url, ".kay", infix);
    if (!download_url) {
        fprintf(stderr, "kaya: '%s' depoda bulunamadı (Packages/%s içinde .kay dosyası yok)\n", name, name);
        return 1;
    }
    char tdir[600];
    if (make_tmpdir(tdir, sizeof(tdir)) != 0) { free(download_url); return 1; }
    char tmp[700];
    snprintf(tmp, sizeof(tmp), "%s/package.kay", tdir);
    printf("İndiriliyor: %s\n", download_url);
    int rc;
    if (download_file(download_url, tmp) != 0) rc = 1;
    else rc = cmd_install(tmp);
    free(download_url);
    rm_recursive(tdir);
    return rc;
}

static int cmd_remove(const char *name) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz paket adı: %s\n", name);
        return 1;
    }
    if (!is_installed(name)) {
        fprintf(stderr, "kaya: '%s' kurulu değil\n", name);
        return 1;
    }
    char apps[600];
    apps_dir(apps, sizeof(apps));
    DIR *d = opendir(apps);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d))) {
            if (ent->d_name[0] == '.' || strcmp(ent->d_name, name) == 0) continue;
            manifest_t other;
            if (load_installed_manifest(ent->d_name, &other) == 0) {
                for (int i = 0; i < other.dep_count; i++) {
                    char dname[64], dop[4], dver[32];
                    parse_dep(other.dependencies[i], dname, sizeof(dname), dop, sizeof(dop), dver, sizeof(dver));
                    if (strcmp(dname, name) == 0) {
                        fprintf(stderr, "kaya: '%s' kaldırılamıyor, '%s' buna bağımlı\n", name, ent->d_name);
                        manifest_free(&other);
                        closedir(d);
                        return 1;
                    }
                }
                manifest_free(&other);
            }
        }
        closedir(d);
    }
    char adir[600];
    app_dir(name, adir, sizeof(adir));
    rm_recursive(adir);
    printf("Kaldırıldı: %s\n", name);
    return 0;
}

/* --- list / info -------------------------------------------------------- */

static int cmd_list(void) {
    char apps[600];
    apps_dir(apps, sizeof(apps));
    DIR *d = opendir(apps);
    if (!d) { printf("Kurulu paket yok.\n"); return 0; }
    struct dirent *ent;
    int count = 0;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] == '.') continue;
        manifest_t m;
        if (load_installed_manifest(ent->d_name, &m) == 0) {
            printf("%-16s %-10s %s\n", m.name, m.version, m.display_name);
            manifest_free(&m);
            count++;
        }
    }
    closedir(d);
    if (count == 0) printf("Kurulu paket yok.\n");
    return 0;
}

static int cmd_info(const char *name) {
    manifest_t m;
    if (load_installed_manifest(name, &m) != 0) {
        fprintf(stderr, "kaya: '%s' kurulu değil\n", name);
        return 1;
    }
    manifest_print(&m);
    manifest_free(&m);
    return 0;
}

/* --- run ---------------------------------------------------------------- */

static int run_html(const char *path) {
    if (browser_override[0]) {
        char *args[] = { browser_override, (char *)path, NULL };
        execvp(browser_override, args);
        fprintf(stderr, "kaya: '%s' tarayıcısı bulunamadı/çalıştırılamadı\n", browser_override);
        return 1;
    }
    const char *candidates[] = { "xdg-open", "sensible-browser", "firefox", "chromium", "google-chrome", NULL };
    for (int i = 0; candidates[i]; i++) {
        char *args[] = { (char *)candidates[i], (char *)path, NULL };
        execvp(candidates[i], args);
    }
    fprintf(stderr, "kaya: açılacak bir tarayıcı bulunamadı, --browser <isim> ile belirt\n");
    return 1;
}

/* Plugin kuralı: <plugins>/<name>/run <entry_abspath> [args...]
 * Başarılıysa geri DÖNMEZ (exec). Dönerse: -1 = plugin kullanılamıyor
 * (kurulu değil / izin yok / run yok), çağıran yedek yönteme geçebilir. */
static int run_via_plugin(const char *plugin, const char *entry_path, int argc, char **argv) {
    if (!is_plugin_installed(plugin)) return -1;
    plugin_info pi;
    if (load_plugin_info(plugin, &pi) != 0) {
        fprintf(stderr, "kaya: plugin '%s' bozuk (plugin.json okunamadı)\n", plugin);
        return -1;
    }
    int allowed = plugin_has_perm(&pi, "run_process");
    plugin_info_free(&pi);
    if (!allowed) {
        fprintf(stderr, "kaya: plugin '%s' 'run_process' iznine sahip değil\n", plugin);
        return -1;
    }
    char runp[800];
    snprintf(runp, sizeof(runp), "%s/plugins/%s/run", root_dir, plugin);
    struct stat st;
    if (lstat(runp, &st) != 0 || !S_ISREG(st.st_mode) || access(runp, X_OK) != 0) {
        fprintf(stderr, "kaya: plugin '%s' içinde çalıştırılabilir 'run' dosyası yok\n", plugin);
        return -1;
    }
    char abs_entry[PATH_MAX];
    if (!realpath(entry_path, abs_entry)) {
        fprintf(stderr, "kaya: '%s' bulunamadı\n", entry_path);
        return -2;
    }
    char *args[64];
    int ai = 0;
    args[ai++] = runp;
    args[ai++] = abs_entry;
    for (int i = 0; i < argc && ai < 62; i++) args[ai++] = argv[i];
    args[ai] = NULL;
    execv(runp, args);
    perror("kaya: plugin çalıştırılamadı");
    return -1;
}

/* Yedek yöntem: .sb3 -> pystage -> python. Shell yok, argv ile exec. */
static int run_sb3(const char *sb3_path, int argc, char **argv) {
    char base[600];
    if (make_tmpdir(base, sizeof(base)) != 0) return 1;
    char tmpdir[700], log_path[700];
    snprintf(tmpdir, sizeof(tmpdir), "%s/proj", base);
    snprintf(log_path, sizeof(log_path), "%s/convert.log", base);

    char abs_sb3[4096];
    if (!realpath(sb3_path, abs_sb3)) {
        fprintf(stderr, "kaya: '%s' bulunamadı\n", sb3_path);
        rm_recursive(base);
        return 1;
    }

    char *conv_argv[] = {
        "python3", "-m", "pystage.convert.sb3",
        abs_sb3, "-l", "en", "-d", tmpdir, "-vv", NULL
    };
    int rc = run_argv(conv_argv, log_path);
    if (rc != 0) {
        fprintf(stderr,
            "kaya: sb3 dönüştürülemedi. 'pystage' kurulu mu? (pip install pystage pygame)\n"
            "      ayrıntı için: %s\n", log_path);
        return 1;
    }

    char found[1100] = "";
    char candidate[1100];
    snprintf(candidate, sizeof(candidate), "%s/main.py", tmpdir);
    struct stat st;
    if (stat(candidate, &st) == 0) {
        snprintf(found, sizeof(found), "%s", candidate);
    } else {
        DIR *d = opendir(tmpdir);
        if (d) {
            struct dirent *ent;
            while ((ent = readdir(d))) {
                size_t len = strlen(ent->d_name);
                if (len > 3 && strcmp(ent->d_name + len - 3, ".py") == 0 &&
                    strcmp(ent->d_name, "__init__.py") != 0) {
                    snprintf(found, sizeof(found), "%s/%s", tmpdir, ent->d_name);
                    break;
                }
            }
            closedir(d);
        }
    }
    if (!found[0]) {
        fprintf(stderr, "kaya: pystage dönüştürmesinden çalıştırılabilir .py bulunamadı (%s)\n", tmpdir);
        rm_recursive(base);
        return 1;
    }

    const char *bn = strrchr(found, '/');
    bn = bn ? bn + 1 : found;
    if (chdir(tmpdir) != 0) {
        perror("kaya: proje dizinine geçilemedi");
        rm_recursive(base);
        return 1;
    }

    char *args[64];
    int ai = 0;
    args[ai++] = "python3";
    args[ai++] = (char *)bn;
    for (int i = 0; i < argc && ai < 62; i++) args[ai++] = argv[i];
    args[ai] = NULL;
    execvp("python3", args);
    perror("kaya: çalıştırılamadı");
    return 1;
}

static int cmd_run(const char *name, int argc, char **argv) {
    if (!is_safe_component(name)) {
        fprintf(stderr, "kaya: geçersiz paket adı: %s\n", name);
        return 1;
    }
    manifest_t m;
    if (load_installed_manifest(name, &m) != 0) {
        fprintf(stderr, "kaya: '%s' kurulu değil\n", name);
        return 1;
    }
    if (!is_safe_rel_path(m.entry)) {
        fprintf(stderr, "kaya: kurulu manifestteki 'entry' alanı geçersiz: %s\n", m.entry);
        manifest_free(&m);
        return 1;
    }
    char adir[600];
    app_dir(name, adir, sizeof(adir));
    char entry_path[900];
    snprintf(entry_path, sizeof(entry_path), "%s/%s", adir, m.entry);
    char runtime[32];
    snprintf(runtime, sizeof(runtime), "%s", m.runtime);
    manifest_free(&m);

    if (strcmp(runtime, "html") == 0) return run_html(entry_path);

    if (strcmp(runtime, "sb3") == 0) {
        /* Scratch plugin (TurboWarp) kuruluysa o; değilse PyStage. */
        int rc = run_via_plugin("scratch", entry_path, argc, argv);
        if (rc == -2) return 1;
        return run_sb3(entry_path, argc, argv);
    }

    if (plugin_only_runtime(runtime)) {
        int rc = run_via_plugin(runtime, entry_path, argc, argv);
        (void)rc; /* başarılıysa zaten geri dönmez */
        fprintf(stderr, "kaya: '%s' çalıştırmak için '%s' plugin'i gerekli: kaya --install -plugin %s\n", runtime, runtime, runtime);
        return 1;
    }

    char *args[64];
    int ai = 0;
    if (strcmp(runtime, "python") == 0) {
        args[ai++] = "python3";
        args[ai++] = entry_path;
    } else if (strcmp(runtime, "sh") == 0 || strcmp(runtime, "shell") == 0) {
        args[ai++] = "sh";
        args[ai++] = entry_path;
    } else if (strcmp(runtime, "node") == 0) {
        args[ai++] = "node";
        args[ai++] = entry_path;
    } else {
        args[ai++] = entry_path;
    }
    for (int i = 0; i < argc && ai < 62; i++) args[ai++] = argv[i];
    args[ai] = NULL;

    execvp(args[0], args);
    perror("kaya: çalıştırılamadı");
    return 1;
}

/* --- upgrade / relook ---------------------------------------------------
 * --upgrade yalnızca kontrol eder ve bildirir; çalışan binary'yi değiştirmez. */
static int cmd_upgrade(void) {
    char tdir[600];
    if (make_tmpdir(tdir, sizeof(tdir)) != 0) return 1;
    char tmp[700];
    snprintf(tmp, sizeof(tmp), "%s/release.json", tdir);
    if (download_file(RELEASES_API, tmp) != 0) { rm_recursive(tdir); return 1; }
    char *text = read_file(tmp);
    rm_recursive(tdir);
    if (!text) { fprintf(stderr, "kaya: sürüm bilgisi okunamadı\n"); return 1; }
    const char *jerr;
    json_value *root = json_parse(text, &jerr);
    free(text);
    if (!root) { fprintf(stderr, "kaya: GitHub yanıtı ayrıştırılamadı: %s\n", jerr ? jerr : "?"); return 1; }
    const char *tag = json_get_string(root, "tag_name", NULL);
    if (!tag) { fprintf(stderr, "kaya: sürüm bilgisi bulunamadı\n"); json_free(root); return 1; }
    const char *cmp_tag = tag;
    if (*cmp_tag == 'v' || *cmp_tag == 'V') cmp_tag++; /* "v1.0-beta.2" == "1.0-beta.2" */
    printf("Kurulu sürüm : %s\n", KAYA_VERSION);
    printf("Son sürüm    : %s\n", tag);
    if (strcmp(cmp_tag, KAYA_VERSION) == 0) printf("Kaya güncel.\n");
    else printf("Yeni sürüm mevcut: %s\n", RELEASES_URL);
    json_free(root);
    return 0;
}

static int cmd_relook(void) {
    mkdirs(root_dir);
    char idx_path[600];
    snprintf(idx_path, sizeof(idx_path), "%s/index.json", root_dir);
    printf("Liste güncelleniyor: %s\n", INDEX_URL);
    if (download_file(INDEX_URL, idx_path) != 0) return 1;
    char *text = read_file(idx_path);
    if (!text) { fprintf(stderr, "kaya: index.json okunamadı\n"); return 1; }
    const char *jerr;
    json_value *idx = json_parse(text, &jerr);
    free(text);
    if (!idx) {
        fprintf(stderr, "kaya: index.json geçersiz: %s\n", jerr ? jerr : "?");
        return 1;
    }
    size_t n = (idx->type == JSON_ARRAY) ? idx->u.array.count : 0;
    printf("Liste güncellendi: %zu girdi\n", n);
    json_free(idx);
    return 0;
}

static void usage(void) {
    fprintf(stderr,
        "kaya %s\n"
        "kullanım:\n"
        "  kaya --install <isim>                    depodan paket kurar\n"
        "  kaya --install -file <dosya.kay>         yerel .kay dosyasını kurar\n"
        "  kaya --install -plugin <isim>            resmi depodan plugin kurar\n"
        "  kaya --remove <isim>\n"
        "  kaya --remove -plugin <isim>\n"
        "  kaya --upgrade\n"
        "  kaya --relook\n"
        "  kaya list\n"
        "  kaya plugins\n"
        "  kaya info <isim>\n"
        "  kaya run <isim> [-- argümanlar...]\n"
        "\n"
        "runtime türleri (manifest.json içindeki \"runtime\" alanı):\n"
        "  native (vars.) doğrudan çalıştırılır\n"
        "  python         python3 <entry>\n"
        "  node           node <entry>\n"
        "  sh / shell     sh <entry>\n"
        "  html           <entry> bir tarayıcıda açılır\n"
        "  sb3            Scratch projesi: 'scratch' plugin'i (TurboWarp), yoksa PyStage\n"
        "  wasm           WebAssembly: 'wasm' plugin'i gerekli\n"
        "  c / cpp        kaynak derlenip çalıştırılır: 'c' / 'cpp' plugin'i gerekli\n"
        "\n"
        "seçenekler:\n"
        "  --root DIR      kurulum kök dizini (varsayılan: %s)\n"
        "  --browser NAME  'run' komutunda html paketleri için tarayıcı (ör. firefox)\n",
        KAYA_VERSION, DEFAULT_ROOT);
}

int main(int argc, char **argv) {
    strncpy(root_dir, DEFAULT_ROOT, sizeof(root_dir) - 1);

    int *argi = calloc((size_t)argc + 1, sizeof(int));
    char **rest = calloc((size_t)argc + 1, sizeof(char *));
    if (!argi || !rest) { fprintf(stderr, "kaya: bellek yetersiz\n"); return 1; }
    int argn = 0;
    char **passthru = NULL;
    int pass_n = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { /* bundan sonrası uygulamaya gider, kaya seçeneği değil */
            passthru = &argv[i + 1];
            pass_n = argc - (i + 1);
            break;
        }
        if (strcmp(argv[i], "--root") == 0 && i + 1 < argc) {
            strncpy(root_dir, argv[i + 1], sizeof(root_dir) - 1);
            i++;
        } else if (strcmp(argv[i], "--browser") == 0 && i + 1 < argc) {
            strncpy(browser_override, argv[i + 1], sizeof(browser_override) - 1);
            i++;
        } else {
            argi[argn++] = i;
        }
    }
    if (argn == 0) { usage(); return 1; }

    const char *cmd = argv[argi[0]];

    if (strcmp(cmd, "--install") == 0) {
        if (argn >= 2 && strcmp(argv[argi[1]], "-fileplugin") == 0) {
            fprintf(stderr, "kaya: '-fileplugin' kaldırıldı. Pluginler yalnızca resmi depodan kurulur: kaya --install -plugin <isim>\n");
            return 1;
        }
        if (argn >= 3 && strcmp(argv[argi[1]], "-file") == 0) return cmd_install(argv[argi[2]]);
        if (argn >= 3 && strcmp(argv[argi[1]], "-plugin") == 0) return cmd_install_plugin_repo(argv[argi[2]]);
        if (argn >= 2) return cmd_install_repo(argv[argi[1]]);
        usage(); return 1;
    }
    if (strcmp(cmd, "--remove") == 0) {
        if (argn >= 3 && strcmp(argv[argi[1]], "-plugin") == 0) return cmd_remove_plugin(argv[argi[2]]);
        if (argn >= 2) return cmd_remove(argv[argi[1]]);
        usage(); return 1;
    }
    if (strcmp(cmd, "--upgrade") == 0) return cmd_upgrade();
    if (strcmp(cmd, "--relook") == 0) return cmd_relook();

    if (strcmp(cmd, "list") == 0) return cmd_list();
    if (strcmp(cmd, "plugins") == 0) return cmd_list_plugins();
    if (strcmp(cmd, "info") == 0 && argn >= 2) return cmd_info(argv[argi[1]]);
    if (strcmp(cmd, "run") == 0 && argn >= 2) {
        int rn = 0;
        for (int i = 2; i < argn; i++) rest[rn++] = argv[argi[i]];
        for (int i = 0; i < pass_n; i++) rest[rn++] = passthru[i];
        return cmd_run(argv[argi[1]], rn, rest);
    }
    usage();
    return 1;
}
