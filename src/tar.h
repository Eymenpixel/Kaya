#ifndef KAYA_TAR_H
#define KAYA_TAR_H

#include <stddef.h>

typedef struct {
    char name[300];
    unsigned long size;
    long data_offset;
} tar_entry;

typedef struct tar_writer tar_writer;

/* Lists the regular-file entries in a USTAR archive. Caller must free()
 * the returned array. Returns NULL (and sets *count = 0) on error
 * (bozuk checksum, dosyadan büyük boyut vb. dahil). */
tar_entry *tar_list(const char *archive_path, size_t *count);

/* Reads a single named entry's data into a newly malloc'd, NUL-terminated
 * buffer (64 MB üst sınır). Caller must free() it. NULL if not found / error. */
unsigned char *tar_read_entry(const char *archive_path, const char *entry_name, unsigned long *out_size);

/* Extracts every entry into dest_dir, preserving directory structure.
 * Unsafe names => -1, nothing written. Any write error => -1 (çağıran
 * dest_dir'i temizlemeli). Returns 0 on success. */
int tar_extract_all(const char *archive_path, const char *dest_dir);

tar_writer *tar_writer_open(const char *archive_path);
int tar_writer_add_file(tar_writer *tw, const char *name_in_archive, const char *source_path);
int tar_writer_close(tar_writer *tw);

#endif
