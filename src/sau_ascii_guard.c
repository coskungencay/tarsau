#include "sau_ascii_guard.h"

#include <stdio.h>

#define SAU_ASCII_SCAN_BUFFER 65536

static inline int byte_is_acceptable(unsigned char value)
{
    if (value == 0x09 || value == 0x0A || value == 0x0D) {
        return 1;
    }
    if (value >= 0x20 && value <= 0x7E) {
        return 1;
    }
    return 0;
}

SauStatus sau_ascii_guard_scan(const char *path, SauAsciiVerdict *verdict)
{
    if (path == NULL || verdict == NULL) {
        return SAU_ERR_IO;
    }
    verdict->byte_count = 0;
    verdict->accepted   = 1;

    FILE *stream = fopen(path, "rb");
    if (stream == NULL) {
        return SAU_ERR_INPUT_MISSING;
    }

    unsigned char buffer[SAU_ASCII_SCAN_BUFFER];
    long long running_count = 0;
    int       still_valid   = 1;

    for (;;) {
        size_t got = fread(buffer, 1, sizeof buffer, stream);
        if (got == 0) {
            if (ferror(stream)) {
                fclose(stream);
                return SAU_ERR_IO;
            }
            break; 
        }
        running_count += (long long)got;

        if (still_valid) {
            for (size_t i = 0; i < got; ++i) {
                if (!byte_is_acceptable(buffer[i])) {
                    still_valid = 0;
                    break;
                }
            }
        }
        
    }

    if (fclose(stream) != 0) {
        return SAU_ERR_IO;
    }

    verdict->byte_count = running_count;
    verdict->accepted   = still_valid;
    return SAU_OK;
}
