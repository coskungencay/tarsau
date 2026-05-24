#include "sau_fs_ops.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * Bu modülde build ve extract'in ortak kullandığı alt seviye yapı
 * taşları var: sınırlı streaming copy, dizin oluşturma ve path
 * birleştirme. Bunların hiçbiri kullanıcıya yazı basmıyor; görevleri
 * yalnızca üst katmanın tepki vereceği bir status döndürmek.
 */

SauStatus sau_fs_stream_exact_bytes(FILE      *source,
                                    FILE      *destination,
                                    long long  byte_count)
{
    if (source == NULL || destination == NULL) {
        return SAU_ERR_IO;
    }
    if (byte_count < 0) {
        return SAU_ERR_IO;
    }

    unsigned char buffer[SAU_STREAM_CHUNK];
    long long remaining = byte_count;

    while (remaining > 0) {
        size_t request = (remaining > (long long)sizeof buffer)
                             ? sizeof buffer
                             : (size_t)remaining;
        size_t got = fread(buffer, 1, request, source);
        if (got == 0) {
            if (ferror(source)) {
                return SAU_ERR_IO;
            }
            /*
             * Beklenen byte sayısı tükenmeden EOF gelmesi, arşiv
             * gövdesinin metadata'nın söylediği uzunluktan kısa olduğu
             * anlamına gelir.
             */
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        size_t written = fwrite(buffer, 1, got, destination);
        if (written != got) {
            return SAU_ERR_IO;
        }
        remaining -= (long long)got;
    }
    return SAU_OK;
}

static SauStatus make_single_directory(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return SAU_OK;
    }
    if (errno == EEXIST) {
        struct stat info;
        if (stat(path, &info) != 0) {
            return SAU_ERR_TARGET_DIR;
        }
        if (!S_ISDIR(info.st_mode)) {
            return SAU_ERR_TARGET_DIR;
        }
        return SAU_OK;
    }
    return SAU_ERR_TARGET_DIR;
}

SauStatus sau_fs_ensure_directory(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return SAU_ERR_TARGET_DIR;
    }

    /*
     * Yolu segment segment dolaşıp her ön eki sırayla yaratıyorum.
     * Ara ön ekleri tutmak için bir scratch buffer var; çağıran 4 KB
     * üstü bir şey verirse sessizce kesmek yerine direkt hata
     * döndürüyorum.
     */
    size_t length = strlen(path);
    if (length >= 4096) {
        return SAU_ERR_TARGET_DIR;
    }

    char scratch[4096];
    memcpy(scratch, path, length);
    scratch[length] = '\0';

    size_t cursor = 0;
    /* Baştaki '/' karakterini atla; ilk segmente bitişik kalsın diye. */
    if (scratch[0] == '/') {
        cursor = 1;
    }

    while (cursor <= length) {
        if (scratch[cursor] == '/' || scratch[cursor] == '\0') {
            char saved = scratch[cursor];
            scratch[cursor] = '\0';
            if (scratch[0] != '\0') {
                SauStatus step = make_single_directory(scratch);
                if (step != SAU_OK) {
                    return step;
                }
            }
            scratch[cursor] = saved;
            if (saved == '\0') {
                break;
            }
        }
        cursor += 1;
    }
    return SAU_OK;
}

SauStatus sau_fs_join_path(const char *directory,
                           const char *name,
                           char       *out_buffer,
                           size_t      out_capacity)
{
    if (out_buffer == NULL || out_capacity == 0 || name == NULL) {
        return SAU_ERR_IO;
    }
    size_t name_length = strlen(name);

    if (directory == NULL || directory[0] == '\0') {
        if (name_length + 1 > out_capacity) {
            return SAU_ERR_IO;
        }
        memcpy(out_buffer, name, name_length);
        out_buffer[name_length] = '\0';
        return SAU_OK;
    }

    size_t dir_length = strlen(directory);
    int    needs_sep  = (dir_length > 0 && directory[dir_length - 1] != '/');
    size_t total      = dir_length + (needs_sep ? 1 : 0) + name_length;
    if (total + 1 > out_capacity) {
        return SAU_ERR_IO;
    }
    memcpy(out_buffer, directory, dir_length);
    size_t offset = dir_length;
    if (needs_sep) {
        out_buffer[offset++] = '/';
    }
    memcpy(out_buffer + offset, name, name_length);
    out_buffer[offset + name_length] = '\0';
    return SAU_OK;
}
