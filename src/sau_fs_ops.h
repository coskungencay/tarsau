#ifndef SAU_FS_OPS_H
#define SAU_FS_OPS_H

#include <stdio.h>
#include <sys/types.h>

#include "sau_error.h"


#define SAU_STREAM_CHUNK 65536


SauStatus sau_fs_stream_exact_bytes(FILE      *source,
                                    FILE      *destination,
                                    long long  byte_count);


SauStatus sau_fs_ensure_directory(const char *path);


SauStatus sau_fs_join_path(const char *directory,
                           const char *name,
                           char       *out_buffer,
                           size_t      out_capacity);

#endif /* SAU_FS_OPS_H */
