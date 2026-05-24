#include "sau_unpack_reader.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sau_blueprint.h"
#include "sau_fs_ops.h"
#include "sau_path_safety.h"


static int has_sau_suffix(const char *path)
{
    if (path == NULL) {
        return 0;
    }
    size_t length = strlen(path);
    const char *expected = SAU_EXTENSION;
    size_t expected_length = strlen(expected);
    if (length < expected_length) {
        return 0;
    }
    return strcmp(path + length - expected_length, expected) == 0;
}

static SauStatus read_header_length(FILE *archive, long long *out_length)
{
    char header[SAU_HEADER_WIDTH];
    size_t got = fread(header, 1, SAU_HEADER_WIDTH, archive);
    if (got != SAU_HEADER_WIDTH) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    long long value = 0;
    for (size_t i = 0; i < SAU_HEADER_WIDTH; ++i) {
        unsigned char c = (unsigned char)header[i];
        if (c < '0' || c > '9') {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        if (value > SAU_MAX_TOTAL_BYTES) {
            return SAU_ERR_ARCHIVE_CORRUPT;
        }
        value = value * 10 + (long long)(c - '0');
    }
    if (value <= 0) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    *out_length = value;
    return SAU_OK;
}

static SauStatus inspect_archive_size(const char *path, long long *out_size)
{
    struct stat info;
    if (stat(path, &info) != 0) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    if (!S_ISREG(info.st_mode)) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    *out_size = (long long)info.st_size;
    return SAU_OK;
}

static SauStatus load_organisation_slice(FILE *archive,
                                         long long organisation_length,
                                         char **out_slice)
{
    if (organisation_length <= 0 || organisation_length > SAU_MAX_TOTAL_BYTES) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    char *slice = (char *)malloc((size_t)organisation_length + 1);
    if (slice == NULL) {
        return SAU_ERR_MEMORY;
    }
    size_t got = fread(slice, 1, (size_t)organisation_length, archive);
    if ((long long)got != organisation_length) {
        free(slice);
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    slice[organisation_length] = '\0';
    *out_slice = slice;
    return SAU_OK;
}

static SauStatus prepare_destination(const char *target_directory)
{
    if (target_directory == NULL || target_directory[0] == '\0') {
        return SAU_OK; /* mevcut çalışma dizini zaten var */
    }
    return sau_fs_ensure_directory(target_directory);
}

static void compose_success_line(const char         *target_directory,
                                 const SauBlueprint *blueprint)
{

    if (target_directory != NULL && target_directory[0] != '\0') {
        printf("%s dizininde ", target_directory);
    } else {
        printf("Geçerli dizinde ");
    }
    for (int i = 0; i < blueprint->count; ++i) {
        if (i == 0) {
            printf("%s", blueprint->entries[i].stored_name);
        } else if (i == blueprint->count - 1) {
            printf(" ve %s", blueprint->entries[i].stored_name);
        } else {
            printf(", %s", blueprint->entries[i].stored_name);
        }
    }
    if (blueprint->count == 1) {
        printf(" dosyası açıldı.\n");
    } else {
        printf(" dosyaları açıldı.\n");
    }
}

static SauStatus materialise_entries(FILE               *archive,
                                     const SauBlueprint *blueprint,
                                     const char         *target_directory)
{
  
    char *created_paths[SAU_MAX_INPUT_FILES] = { NULL };
    int   created_count = 0;
    SauStatus outcome = SAU_OK;

    for (int i = 0; i < blueprint->count; ++i) {
        const SauEntry *entry = &blueprint->entries[i];

        char *joined = (char *)malloc(4096);
        if (joined == NULL) {
            outcome = SAU_ERR_MEMORY;
            break;
        }
        SauStatus join_status = sau_fs_join_path(target_directory,
                                                 entry->stored_name,
                                                 joined,
                                                 4096);
        if (join_status != SAU_OK) {
            free(joined);
            outcome = join_status;
            break;
        }

        FILE *destination = fopen(joined, "wb");
        if (destination == NULL) {
            free(joined);
            outcome = SAU_ERR_IO;
            break;
        }
        SauStatus stream = sau_fs_stream_exact_bytes(archive, destination,
                                                     entry->byte_count);
        if (fclose(destination) != 0 && stream == SAU_OK) {
            stream = SAU_ERR_IO;
        }
        if (stream != SAU_OK) {
            free(joined);
            outcome = stream;
            break;
        }

        if (chmod(joined, entry->permission_bits) != 0) {
            
            outcome = SAU_ERR_IO;
            created_paths[created_count++] = joined;
            break;
        }
        created_paths[created_count++] = joined;
    }

    if (outcome != SAU_OK) {
        for (int j = 0; j < created_count; ++j) {
            if (created_paths[j] != NULL) {
                remove(created_paths[j]);
            }
        }
    }
    for (int j = 0; j < created_count; ++j) {
        free(created_paths[j]);
    }
    return outcome;
}

static SauStatus check_no_trailing_bytes(FILE *archive)
{
    /* Beyan edilen payload'ların dışında byte kalmışsa arşiv hatalı demektir. */
    int sentinel = fgetc(archive);
    if (sentinel != EOF) {
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    return SAU_OK;
}

SauStatus sau_unpack_restore_bundle(const SauCommand *command)
{
    if (command == NULL || command->mode != SAU_MODE_EXTRACT) {
        return SAU_ERR_USAGE;
    }
    if (command->archive_to_open == NULL) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    if (!has_sau_suffix(command->archive_to_open)) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    long long archive_size = 0;
    if (inspect_archive_size(command->archive_to_open, &archive_size) != SAU_OK) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    if (archive_size < (long long)(SAU_HEADER_WIDTH + 2)) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    FILE *archive = fopen(command->archive_to_open, "rb");
    if (archive == NULL) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    long long organisation_length = 0;
    if (read_header_length(archive, &organisation_length) != SAU_OK) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }
    if (organisation_length + SAU_HEADER_WIDTH > archive_size) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    char *organisation_slice = NULL;
    if (load_organisation_slice(archive, organisation_length,
                                &organisation_slice) != SAU_OK) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    SauBlueprint blueprint;
    sau_blueprint_reset(&blueprint);
    SauStatus parsed = sau_blueprint_parse_records(&blueprint,
                                                   organisation_slice,
                                                   (size_t)organisation_length);
    free(organisation_slice);
    if (parsed != SAU_OK) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    long long expected_total = (long long)SAU_HEADER_WIDTH
                             + organisation_length
                             + blueprint.payload_total_bytes;
    if (expected_total != archive_size) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    /*
     * Ancak şimdi, arşiv tamamen doğrulandıktan sonra, kullanıcının
     * dosya sistemine yazmaya başlıyorum.
     */
    if (prepare_destination(command->target_directory) != SAU_OK) {
        fclose(archive);
        sau_error_report_generic("Hata: hedef dizin hazırlanamadı.");
        return SAU_ERR_TARGET_DIR;
    }

    SauStatus materialised = materialise_entries(archive, &blueprint,
                                                 command->target_directory);
    if (materialised != SAU_OK) {
        fclose(archive);
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    SauStatus trailing = check_no_trailing_bytes(archive);
    if (fclose(archive) != 0 && trailing == SAU_OK) {
        trailing = SAU_ERR_IO;
    }
    if (trailing != SAU_OK) {
        sau_error_report_bad_archive();
        return SAU_ERR_ARCHIVE_CORRUPT;
    }

    compose_success_line(command->target_directory, &blueprint);
    return SAU_OK;
}
