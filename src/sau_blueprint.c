#include "sau_blueprint.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sau_path_safety.h"

/*
 *
 *   record   := name ',' permission ',' size
 *   archive  := '|' record '|' record '|' ... record '|'
 *
 * Pipe karakteri hem açıcı, hem ardışık iki kayıt arasında paylaşılan
 * sınır, hem de en sondaki kapanış. Yani `pipe_sayisi == entry + 1`.
 * İki giriş şöyle render olur: `|name1,perm1,size1|name2,perm2,size2|`.
 */

#define SAU_RECORD_NAME_CAP   (SAU_MAX_STORED_NAME)
#define SAU_RECORD_PERM_CAP   8
#define SAU_RECORD_SIZE_CAP   24
#define SAU_RECORD_MAX_BYTES  (1 + SAU_RECORD_NAME_CAP + 1 + SAU_RECORD_PERM_CAP + 1 + SAU_RECORD_SIZE_CAP + 1)

void sau_blueprint_reset(SauBlueprint *blueprint)
{
    if (blueprint == NULL) {
        return;
    }
    blueprint->count = 0;
    blueprint->payload_total_bytes = 0;
    for (int i = 0; i < SAU_MAX_INPUT_FILES; ++i) {
        blueprint->entries[i].stored_name[0] = '\0';
        blueprint->entries[i].permission_bits = 0;
        blueprint->entries[i].byte_count = 0;
    }
}

SauStatus sau_blueprint_append(SauBlueprint *blueprint,
                               const char   *stored_name,
                               mode_t        permission_bits,
                               long long     byte_count)
{
    if (blueprint == NULL || stored_name == NULL) {
        return SAU_ERR_USAGE;
    }
    if (blueprint->count >= SAU_MAX_INPUT_FILES) {
        return SAU_ERR_TOO_MANY_FILES;
    }
    if (byte_count < 0) {
        return SAU_ERR_USAGE;
    }
    size_t name_length = strlen(stored_name);
    if (name_length == 0 || name_length > SAU_MAX_STORED_NAME) {
        return SAU_ERR_PATH_UNSAFE;
    }

    SauEntry *slot = &blueprint->entries[blueprint->count];
    memcpy(slot->stored_name, stored_name, name_length);
    slot->stored_name[name_length] = '\0';
    slot->permission_bits = (mode_t)(permission_bits & 0777);
    slot->byte_count      = byte_count;

    blueprint->count               += 1;
    blueprint->payload_total_bytes += byte_count;
    return SAU_OK;
}

static size_t append_to_buffer(char *buffer, size_t capacity, size_t cursor,
                               const char *fragment, size_t fragment_length)
{
    if (cursor + fragment_length > capacity) {
        return capacity + 1; /* taşma sinyali */
    }
    memcpy(buffer + cursor, fragment, fragment_length);
    return cursor + fragment_length;
}

SauStatus sau_blueprint_render_records(const SauBlueprint *blueprint,
                                       char              **out_buffer,
                                       size_t             *out_length)
{
    if (blueprint == NULL || out_buffer == NULL || out_length == NULL) {
        return SAU_ERR_USAGE;
    }
    *out_buffer = NULL;
    *out_length = 0;

    if (blueprint->count == 0) {
        return SAU_ERR_USAGE;
    }

    size_t capacity = (size_t)blueprint->count * SAU_RECORD_MAX_BYTES + 4;
    char  *buffer   = (char *)malloc(capacity);
    if (buffer == NULL) {
        return SAU_ERR_MEMORY;
    }

    size_t cursor = append_to_buffer(buffer, capacity, 0, "|", 1);
    if (cursor > capacity) goto overflow;

    for (int i = 0; i < blueprint->count; ++i) {
        const SauEntry *entry = &blueprint->entries[i];

        size_t name_length = strlen(entry->stored_name);
        cursor = append_to_buffer(buffer, capacity, cursor, entry->stored_name, name_length);
        if (cursor > capacity) goto overflow;

        char permission_text[8];
        int  permission_written = snprintf(permission_text, sizeof permission_text,
                                           ",%04o,", (unsigned int)(entry->permission_bits & 0777));
        if (permission_written < 0) goto overflow;
        cursor = append_to_buffer(buffer, capacity, cursor, permission_text, (size_t)permission_written);
        if (cursor > capacity) goto overflow;

        char size_text[24];
        int  size_written = snprintf(size_text, sizeof size_text, "%lld", entry->byte_count);
        if (size_written < 0) goto overflow;
        cursor = append_to_buffer(buffer, capacity, cursor, size_text, (size_t)size_written);
        if (cursor > capacity) goto overflow;

        cursor = append_to_buffer(buffer, capacity, cursor, "|", 1);
        if (cursor > capacity) goto overflow;
    }

    buffer[cursor] = '\0';
    *out_buffer = buffer;
    *out_length = cursor;
    return SAU_OK;

overflow:
    free(buffer);
    return SAU_ERR_MEMORY;
}

/* ----- parser ------------------------------------------------------------- */
/* Buradan sonrası organizasyon dilimini okuyup blueprint'e dolduran kısım. */

static SauStatus parse_decimal_long_long(const char *text, size_t length,
                                         long long *out_value)
{
    if (length == 0) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    long long acc = 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < '0' || c > '9') {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        if (acc > SAU_MAX_TOTAL_BYTES / 10) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        acc = acc * 10 + (long long)(c - '0');
        if (acc > SAU_MAX_TOTAL_BYTES) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
    }
    *out_value = acc;
    return SAU_OK;
}

static SauStatus parse_octal_permission(const char *text, size_t length,
                                        mode_t *out_value)
{
    if (length == 0 || length > 4) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    unsigned int acc = 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < '0' || c > '7') {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        acc = (acc << 3) | (unsigned int)(c - '0');
    }
    if (acc > 0777) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    *out_value = (mode_t)acc;
    return SAU_OK;
}

SauStatus sau_blueprint_parse_records(SauBlueprint *blueprint,
                                      const char   *slice,
                                      size_t        length)
{
    if (blueprint == NULL || slice == NULL) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    sau_blueprint_reset(blueprint);

    /* Mümkün olan en kısa kayıt `|a,0,0|` yani 7 byte; daha sıkı
     * "boş payload" kontrolü aşağıda dolaylı olarak yapılıyor. */
    if (length < 2 || slice[0] != '|' || slice[length - 1] != '|') {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    size_t cursor = 1; /* açıcı pipe'tan hemen sonraki konum */
    while (cursor < length) {
        /* bu kaydı kapatan pipe'ı bul */
        size_t scan = cursor;
        while (scan < length && slice[scan] != '|') {
            ++scan;
        }
        if (scan == length) {
            /* beklenmedik durum: her kayıt mutlaka kapanmalı */
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        size_t record_length = scan - cursor;
        if (record_length == 0) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        /* iki virgül üzerinden parçala */
        size_t first_comma = cursor;
        while (first_comma < scan && slice[first_comma] != ',') {
            ++first_comma;
        }
        if (first_comma == scan) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        size_t second_comma = first_comma + 1;
        while (second_comma < scan && slice[second_comma] != ',') {
            ++second_comma;
        }
        if (second_comma == scan) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        size_t name_start   = cursor;
        size_t name_length  = first_comma - cursor;
        size_t perm_start   = first_comma + 1;
        size_t perm_length  = second_comma - perm_start;
        size_t size_start   = second_comma + 1;
        size_t size_length  = scan - size_start;

        if (name_length == 0 || name_length > SAU_MAX_STORED_NAME) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        if (perm_length == 0 || size_length == 0) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        char name_buffer[SAU_MAX_STORED_NAME + 1];
        memcpy(name_buffer, slice + name_start, name_length);
        name_buffer[name_length] = '\0';

        if (sau_path_reject_unsafe_member(name_buffer) != SAU_OK) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        mode_t permission_bits = 0;
        if (parse_octal_permission(slice + perm_start, perm_length, &permission_bits) != SAU_OK) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        long long byte_count = 0;
        if (parse_decimal_long_long(slice + size_start, size_length, &byte_count) != SAU_OK) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        SauStatus appended = sau_blueprint_append(blueprint, name_buffer, permission_bits, byte_count);
        if (appended != SAU_OK) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }

        /* kapanış pipe'ını geç; varsa sonraki kayıt buradan başlar */
        cursor = scan + 1;
    }
    if (blueprint->count == 0) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    if (blueprint->payload_total_bytes > SAU_MAX_TOTAL_BYTES) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    return SAU_OK;
}
