#include "tar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCK 512
#define TAR_MAX_READ (64UL * 1024UL * 1024UL) /* tar_read_entry üst sınırı */

struct ustar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

static unsigned long oct2ul(const char *s, size_t len) {
    unsigned long v = 0;
    size_t i = 0;
    while (i < len && s[i] == ' ') i++; /* bazı tar'lar baştaki boşlukla yazar */
    for (; i < len && s[i]; i++) {
        if (s[i] < '0' || s[i] > '7') break;
        v = v * 8 + (s[i] - '0');
    }
    return v;
}

static void ul2oct(char *dst, size_t dstlen, unsigned long v) {
    for (int i = (int)dstlen - 2; i >= 0; i--) {
        dst[i] = '0' + (v & 7);
        v >>= 3;
    }
    dst[dstlen - 1] = '\0';
}

static int is_zero_block(const unsigned char *b) {
    for (int i = 0; i < BLOCK; i++) if (b[i] != 0) return 0;
    return 1;
}

/* Başlık checksum'ı: chksum alanı boşluk sayılarak tüm baytların toplamı. */
static int header_checksum_ok(const unsigned char *b) {
    unsigned long sum = 0;
    for (int i = 0; i < BLOCK; i++) sum += (i >= 148 && i < 156) ? ' ' : b[i];
    const struct ustar_header *h = (const struct ustar_header *)b;
    return sum == oct2ul(h->chksum, sizeof(h->chksum));
}

static long file_size(FILE *f) {
    long cur = ftell(f);
    if (cur < 0) return -1;
    if (fseek(f, 0, SEEK_END) != 0) return -1;
    long sz = ftell(f);
    if (fseek(f, cur, SEEK_SET) != 0) return -1;
    return sz;
}

/* Rejects absolute paths and any ".." path segment, so a tar entry can
 * never escape the extraction directory. */
static int is_safe_entry_name(const char *name) {
    if (!name || !*name) return 0;
    if (name[0] == '/') return 0;
    size_t len = strlen(name);
    if (len == 2 && name[0] == '.' && name[1] == '.') return 0;
    const char *p = name;
    while ((p = strstr(p, "..")) != NULL) {
        int before_ok = (p == name) || (p[-1] == '/');
        int after_ok = (p[2] == '\0') || (p[2] == '/');
        if (before_ok && after_ok) return 0;
        p += 2;
    }
    return 1;
}

static void mkdirs_for(const char *dest_dir, const char *relpath) {
    char full[2048];
    snprintf(full, sizeof(full), "%s/%s", dest_dir, relpath);
    for (char *p = full + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(full, 0755);
            *p = '/';
        }
    }
}

tar_entry *tar_list(const char *archive_path, size_t *count) {
    if (count) *count = 0;
    FILE *f = fopen(archive_path, "rb");
    if (!f) return NULL;
    long fsize = file_size(f);
    if (fsize < 0) { fclose(f); return NULL; }

    size_t cap = 8, n = 0;
    tar_entry *entries = malloc(cap * sizeof(tar_entry));
    if (!entries) { fclose(f); return NULL; }
    unsigned char block[BLOCK];
    long offset = 0;
    while (fread(block, 1, BLOCK, f) == BLOCK) {
        offset += BLOCK;
        if (is_zero_block(block)) break;
        struct ustar_header *h = (struct ustar_header *)block;
        if (!header_checksum_ok(block)) goto fail; /* bozuk / tar olmayan dosya */
        unsigned long size = oct2ul(h->size, sizeof(h->size));
        /* Başlıktaki boyuta güvenme: veri dosyanın içine sığmalı. */
        if (size > (unsigned long)(fsize - offset)) goto fail;
        if (h->typeflag[0] == '0' || h->typeflag[0] == '\0') {
            if (n == cap) {
                cap *= 2;
                tar_entry *ne = realloc(entries, cap * sizeof(tar_entry));
                if (!ne) goto fail;
                entries = ne;
            }
            char full_name[300] = {0};
            if (h->prefix[0]) snprintf(full_name, sizeof(full_name), "%.155s/%.100s", h->prefix, h->name);
            else snprintf(full_name, sizeof(full_name), "%.100s", h->name);
            strncpy(entries[n].name, full_name, sizeof(entries[n].name) - 1);
            entries[n].name[sizeof(entries[n].name) - 1] = '\0';
            entries[n].size = size;
            entries[n].data_offset = offset;
            n++;
        }
        unsigned long padded = ((size + BLOCK - 1) / BLOCK) * BLOCK;
        if (fseek(f, (long)padded, SEEK_CUR) != 0) goto fail;
        offset += (long)padded;
    }
    fclose(f);
    if (count) *count = n;
    return entries;

fail:
    free(entries);
    fclose(f);
    if (count) *count = 0;
    return NULL;
}

unsigned char *tar_read_entry(const char *archive_path, const char *entry_name, unsigned long *out_size) {
    size_t count;
    tar_entry *entries = tar_list(archive_path, &count);
    if (!entries) return NULL;
    unsigned char *data = NULL;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, entry_name) != 0) continue;
        if (entries[i].size > TAR_MAX_READ) break;
        FILE *f = fopen(archive_path, "rb");
        if (!f) break;
        if (fseek(f, entries[i].data_offset, SEEK_SET) == 0) {
            unsigned char *buf = malloc(entries[i].size + 1);
            if (buf) {
                size_t got = fread(buf, 1, entries[i].size, f);
                if (got == entries[i].size) {
                    buf[got] = '\0';
                    if (out_size) *out_size = entries[i].size;
                    data = buf;
                } else {
                    free(buf);
                }
            }
        }
        fclose(f);
        break;
    }
    free(entries);
    return data;
}

/* Fails closed: herhangi bir girdi güvensizse hiçbir şey yazılmadan reddedilir.
 * Yazma sırasında herhangi bir hata olursa -1 döner (sessiz yarım kurulum yok);
 * çağıran taraf hedef dizini temizlemekle yükümlüdür. */
int tar_extract_all(const char *archive_path, const char *dest_dir) {
    size_t count;
    tar_entry *entries = tar_list(archive_path, &count);
    if (!entries) return -1;

    for (size_t i = 0; i < count; i++) {
        if (!is_safe_entry_name(entries[i].name)) {
            fprintf(stderr, "tar: güvensiz girdi adı reddedildi: %s\n", entries[i].name);
            free(entries);
            return -1;
        }
    }

    if (mkdir(dest_dir, 0755) != 0 && errno != EEXIST) { free(entries); return -1; }
    FILE *f = fopen(archive_path, "rb");
    if (!f) { free(entries); return -1; }

    int rc = 0;
    for (size_t i = 0; i < count && rc == 0; i++) {
        mkdirs_for(dest_dir, entries[i].name);
        char full[2048];
        if (snprintf(full, sizeof(full), "%s/%s", dest_dir, entries[i].name) >= (int)sizeof(full)) { rc = -1; break; }
        FILE *out = fopen(full, "wb");
        if (!out) { fprintf(stderr, "tar: yazılamadı: %s\n", full); rc = -1; break; }
        if (fseek(f, entries[i].data_offset, SEEK_SET) != 0) rc = -1;
        unsigned char buf[8192];
        unsigned long remaining = entries[i].size;
        while (rc == 0 && remaining > 0) {
            size_t chunk = remaining < sizeof(buf) ? remaining : sizeof(buf);
            size_t got = fread(buf, 1, chunk, f);
            if (got == 0) { rc = -1; break; }
            if (fwrite(buf, 1, got, out) != got) { rc = -1; break; }
            remaining -= got;
        }
        if (fclose(out) != 0) rc = -1;
    }
    fclose(f);
    free(entries);
    return rc;
}

struct tar_writer {
    FILE *f;
};

tar_writer *tar_writer_open(const char *archive_path) {
    FILE *f = fopen(archive_path, "wb");
    if (!f) return NULL;
    tar_writer *tw = malloc(sizeof(tar_writer));
    if (!tw) { fclose(f); return NULL; }
    tw->f = f;
    return tw;
}

int tar_writer_add_file(tar_writer *tw, const char *name_in_archive, const char *source_path) {
    if (strlen(name_in_archive) >= sizeof(((struct ustar_header *)0)->name)) return -1; /* sessizce kırpma */
    FILE *in = fopen(source_path, "rb");
    if (!in) return -1;
    long size = file_size(in);
    if (size < 0) { fclose(in); return -1; }

    struct ustar_header h;
    memset(&h, 0, sizeof(h));
    strncpy(h.name, name_in_archive, sizeof(h.name) - 1);
    ul2oct(h.mode, sizeof(h.mode), 0644);
    ul2oct(h.uid, sizeof(h.uid), 0);
    ul2oct(h.gid, sizeof(h.gid), 0);
    ul2oct(h.size, sizeof(h.size), (unsigned long)size);
    ul2oct(h.mtime, sizeof(h.mtime), (unsigned long)0);
    memset(h.chksum, ' ', sizeof(h.chksum));
    h.typeflag[0] = '0';
    memcpy(h.magic, "ustar", 5);
    h.magic[5] = 0;
    h.version[0] = '0';
    h.version[1] = '0';

    unsigned char *raw = (unsigned char *)&h;
    unsigned long sum = 0;
    for (size_t i = 0; i < sizeof(h); i++) sum += raw[i];
    snprintf(h.chksum, sizeof(h.chksum), "%06lo", sum);
    h.chksum[6] = '\0';
    h.chksum[7] = ' ';

    if (fwrite(&h, 1, sizeof(h), tw->f) != sizeof(h)) { fclose(in); return -1; }

    unsigned char buf[8192];
    size_t got;
    while ((got = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, got, tw->f) != got) { fclose(in); return -1; }
    }
    fclose(in);

    long padded = ((size + BLOCK - 1) / BLOCK) * BLOCK;
    long pad = padded - size;
    unsigned char zero[BLOCK] = {0};
    if (pad > 0 && fwrite(zero, 1, pad, tw->f) != (size_t)pad) return -1;
    return 0;
}

int tar_writer_close(tar_writer *tw) {
    unsigned char zero[BLOCK] = {0};
    int rc = 0;
    if (fwrite(zero, 1, BLOCK, tw->f) != BLOCK) rc = -1;
    if (fwrite(zero, 1, BLOCK, tw->f) != BLOCK) rc = -1;
    if (fclose(tw->f) != 0) rc = -1;
    free(tw);
    return rc;
}
