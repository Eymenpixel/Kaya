/* mkkay - packs a directory into a .kay file (a plain ustar archive).
 * Usage: mkkay <source_dir> <output.kay>
 * Every regular file directly under source_dir is added. manifest.json is required. */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "tar.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "kullanım: mkkay <kaynak_dizin> <cikti.kay>\n");
        return 1;
    }
    const char *src = argv[1];
    const char *out = argv[2];

    char manifest_path[600];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", src);
    struct stat st;
    if (stat(manifest_path, &st) != 0) {
        fprintf(stderr, "mkkay: %s içinde manifest.json yok\n", src);
        return 1;
    }

    DIR *d = opendir(src);
    if (!d) {
        fprintf(stderr, "mkkay: %s açılamadı\n", src);
        return 1;
    }
    tar_writer *tw = tar_writer_open(out);
    if (!tw) {
        fprintf(stderr, "mkkay: %s oluşturulamadı\n", out);
        closedir(d);
        return 1;
    }

    struct dirent *ent;
    int count = 0, failed = 0;
    while ((ent = readdir(d))) {
        if (ent->d_name[0] == '.') continue;
        char full[700];
        snprintf(full, sizeof(full), "%s/%s", src, ent->d_name);
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (tar_writer_add_file(tw, ent->d_name, full) != 0) {
            fprintf(stderr, "mkkay: eklenemedi: %s\n", ent->d_name);
            failed = 1;
            continue;
        }
        printf("  + %s\n", ent->d_name);
        count++;
    }
    closedir(d);
    if (tar_writer_close(tw) != 0) failed = 1;
    if (failed) { fprintf(stderr, "mkkay: hata oluştu, %s eksik olabilir\n", out); return 1; }
    printf("%s oluşturuldu (%d dosya)\n", out, count);
    return 0;
}
