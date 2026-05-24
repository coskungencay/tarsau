#include "sau_path_safety.h"

#include <string.h>

/*
 * Bu dosyadaki iki yardımcı, arşivin (veya argv'nin) ne istediği
 * fark etmeksizin son dosya yazma çağrısının her zaman düz, dizin
 * dışına çıkmayan ve extract hedefinin altına çakılı kalan bir
 * basename ile yapılmasını garanti ediyor.
 */

static int contains_control_byte(const char *text, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 0x20 || c == 0x7F) {
            return 1;
        }
    }
    return 0;
}

static const char *locate_trailing_segment(const char *raw_path)
{
    if (raw_path == NULL) {
        return NULL;
    }
    const char *cursor = raw_path;
    const char *last_segment = raw_path;
    while (*cursor != '\0') {
        if (*cursor == '/') {
            last_segment = cursor + 1;
        }
        ++cursor;
    }
    return last_segment;
}

SauStatus sau_path_make_stored_basename(const char *raw_path,
                                        char       *out_buffer,
                                        size_t      out_capacity)
{
    if (out_buffer == NULL || out_capacity == 0) {
        return SAU_ERR_PATH_UNSAFE;
    }
    const char *segment = locate_trailing_segment(raw_path);
    if (segment == NULL) {
        return SAU_ERR_PATH_UNSAFE;
    }

    size_t segment_length = strlen(segment);
    if (segment_length == 0) {
        return SAU_ERR_PATH_UNSAFE;
    }
    if (segment_length >= out_capacity) {
        return SAU_ERR_PATH_UNSAFE;
    }
    if (segment_length > SAU_MAX_STORED_NAME) {
        return SAU_ERR_PATH_UNSAFE;
    }
    if (strcmp(segment, ".") == 0 || strcmp(segment, "..") == 0) {
        return SAU_ERR_PATH_UNSAFE;
    }
    if (contains_control_byte(segment, segment_length)) {
        return SAU_ERR_PATH_UNSAFE;
    }
    /*
     * Uzunluğu zaten bildiğim için strcpy/strncpy yerine memcpy + elle
     * NUL tercih ettim; terminatör konusunda hiçbir belirsizlik
     * kalmıyor.
     */
    memcpy(out_buffer, segment, segment_length);
    out_buffer[segment_length] = '\0';
    return SAU_OK;
}

SauStatus sau_path_reject_unsafe_member(const char *member_name)
{
    if (member_name == NULL) {
        return SAU_ERR_PATH_UNSAFE;
    }
    size_t length = strlen(member_name);
    if (length == 0 || length > SAU_MAX_STORED_NAME) {
        return SAU_ERR_PATH_UNSAFE;
    }
    if (strcmp(member_name, ".") == 0 || strcmp(member_name, "..") == 0) {
        return SAU_ERR_PATH_UNSAFE;
    }
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)member_name[i];
        if (c == '/' || c == '\\') {
            return SAU_ERR_PATH_UNSAFE;
        }
        if (c < 0x20 || c == 0x7F) {
            return SAU_ERR_PATH_UNSAFE;
        }
    }
    return SAU_OK;
}
