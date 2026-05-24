#include "sau_pack_writer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sau_ascii_guard.h"
#include "sau_blueprint.h"
#include "sau_fs_ops.h"
#include "sau_path_safety.h"


static SauStatus collect_inputs(const SauCommand *command,
                                SauBlueprint     *blueprint)
{
    for (int i = 0; i < command->input_count; ++i) {
        const char *raw_path = command->input_paths[i];

        struct stat info;
        if (stat(raw_path, &info) != 0) {
            sau_error_report_generic("Hata: giriş dosyası açılamadı.");
            fprintf(stderr, "  -> %s\n", raw_path);
            return SAU_ERR_INPUT_MISSING;
        }
        if (!S_ISREG(info.st_mode)) {
            sau_error_report_bad_input(raw_path);
            return SAU_ERR_INPUT_FORMAT;
        }

        SauAsciiVerdict verdict;
        SauStatus scan_status = sau_ascii_guard_scan(raw_path, &verdict);
        if (scan_status != SAU_OK) {
            sau_error_report_generic("Hata: giriş dosyası okunamadı.");
            fprintf(stderr, "  -> %s\n", raw_path);
            return scan_status;
        }
        if (!verdict.accepted) {
            sau_error_report_bad_input(raw_path);
            return SAU_ERR_INPUT_FORMAT;
        }

        char stored_name[SAU_MAX_STORED_NAME + 1];
        SauStatus naming = sau_path_make_stored_basename(raw_path,
                                                         stored_name,
                                                         sizeof stored_name);
        if (naming != SAU_OK) {
            sau_error_report_generic("Hata: giriş dosyası adı arşiv için uygun değil.");
            fprintf(stderr, "  -> %s\n", raw_path);
            return naming;
        }

        /*
         * Arşive sadece basename yazıldığı için iki farklı yolun
         * basename'i çakışırsa (`dir1/t1` ve `dir2/t1` gibi) extract
         * sırasında biri diğerini ezerdi. Sessiz üzerine yazma yerine
         * baştan reddedip kullanıcıya net sinyal veriyorum.
         */
        for (int prior = 0; prior < blueprint->count; ++prior) {
            if (strcmp(blueprint->entries[prior].stored_name, stored_name) == 0) {
                sau_error_report_generic(
                    "Hata: aynı dosya adı birden fazla kez verilemez.");
                fprintf(stderr, "  -> %s\n", stored_name);
                return SAU_ERR_USAGE;
            }
        }

        SauStatus appended = sau_blueprint_append(blueprint,
                                                  stored_name,
                                                  info.st_mode & 0777,
                                                  verdict.byte_count);
        if (appended == SAU_ERR_TOO_MANY_FILES) {
            sau_error_report_generic(
                "Hata: en fazla 32 giriş dosyası kabul edilir.");
            return appended;
        }
        if (appended != SAU_OK) {
            sau_error_report_generic("Hata: arşiv kaydı oluşturulamadı.");
            return appended;
        }

        if (blueprint->payload_total_bytes > SAU_MAX_TOTAL_BYTES) {
            sau_error_report_generic(
                "Hata: giriş dosyalarının toplam boyutu 200 MB sınırını aşıyor.");
            return SAU_ERR_SIZE_LIMIT;
        }
    }
    return SAU_OK;
}

static SauStatus craft_temp_archive_name(const char *final_name,
                                         char       *buffer,
                                         size_t      capacity)
{
    if (final_name == NULL || buffer == NULL || capacity == 0) {
        return SAU_ERR_IO;
    }
    /*
     * mkstemp() kullanmıyorum; geçici dosyanın son dosyanın yanı
     * başında olmasını istiyorum ki rename() aynı dosya sistemi
     * içinde kalsın. Pid + sabit sonek tipik kullanım için yeterince
     * benzersiz oluyor, taşınabilir bir random-suffix bağımlılığına
     * gerek kalmıyor.
     */
    int produced = snprintf(buffer, capacity, "%s.tmp.%ld", final_name, (long)getpid());
    if (produced < 0 || (size_t)produced >= capacity) {
        return SAU_ERR_IO;
    }
    return SAU_OK;
}

static SauStatus emit_archive(const SauCommand   *command,
                              const SauBlueprint *blueprint,
                              const char         *organisation_bytes,
                              size_t              organisation_length)
{
    char temp_path[4096];
    SauStatus name_status = craft_temp_archive_name(command->output_archive,
                                                    temp_path,
                                                    sizeof temp_path);
    if (name_status != SAU_OK) {
        sau_error_report_generic("Hata: geçici arşiv adı oluşturulamadı.");
        return name_status;
    }

    FILE *archive = fopen(temp_path, "wb");
    if (archive == NULL) {
        sau_error_report_generic("Hata: arşiv dosyası yazıma açılamadı.");
        return SAU_ERR_IO;
    }

    /* 10 byte sabit genişlikli başlık, parser'a kolaylık olsun diye sıfır dolgulu. */
    char header[SAU_HEADER_WIDTH + 1];
    int  header_written = snprintf(header, sizeof header, "%010zu", organisation_length);
    if (header_written != SAU_HEADER_WIDTH) {
        fclose(archive);
        remove(temp_path);
        sau_error_report_generic("Hata: arşiv başlığı oluşturulamadı.");
        return SAU_ERR_IO;
    }
    if (fwrite(header, 1, SAU_HEADER_WIDTH, archive) != SAU_HEADER_WIDTH) {
        fclose(archive);
        remove(temp_path);
        sau_error_report_generic("Hata: arşiv başlığı yazılamadı.");
        return SAU_ERR_IO;
    }
    if (organisation_length > 0 &&
        fwrite(organisation_bytes, 1, organisation_length, archive) != organisation_length) {
        fclose(archive);
        remove(temp_path);
        sau_error_report_generic("Hata: arşiv metadata bölümü yazılamadı.");
        return SAU_ERR_IO;
    }

    for (int i = 0; i < blueprint->count; ++i) {
        const char *source_path = command->input_paths[i];
        FILE *source = fopen(source_path, "rb");
        if (source == NULL) {
            fclose(archive);
            remove(temp_path);
            sau_error_report_generic("Hata: giriş dosyası kopyalama için açılamadı.");
            fprintf(stderr, "  -> %s\n", source_path);
            return SAU_ERR_IO;
        }
        SauStatus stream = sau_fs_stream_exact_bytes(source, archive,
                                                     blueprint->entries[i].byte_count);
        fclose(source);
        if (stream != SAU_OK) {
            fclose(archive);
            remove(temp_path);
            sau_error_report_generic("Hata: arşiv gövdesine yazım başarısız.");
            return stream;
        }
    }

    if (fclose(archive) != 0) {
        remove(temp_path);
        sau_error_report_generic("Hata: arşiv dosyası kapatılamadı.");
        return SAU_ERR_IO;
    }
    if (rename(temp_path, command->output_archive) != 0) {
        remove(temp_path);
        sau_error_report_generic("Hata: arşiv son adına taşınamadı.");
        return SAU_ERR_IO;
    }
    return SAU_OK;
}

SauStatus sau_pack_write_bundle(const SauCommand *command)
{
    if (command == NULL || command->mode != SAU_MODE_BUILD) {
        return SAU_ERR_USAGE;
    }
    if (command->input_count == 0) {
        sau_error_report_generic("Hata: -b modunda en az bir giriş dosyası gerekir.");
        return SAU_ERR_USAGE;
    }

    SauBlueprint blueprint;
    sau_blueprint_reset(&blueprint);

    SauStatus collect = collect_inputs(command, &blueprint);
    if (collect != SAU_OK) {
        return collect;
    }

    char  *organisation_bytes  = NULL;
    size_t organisation_length = 0;
    SauStatus render = sau_blueprint_render_records(&blueprint,
                                                    &organisation_bytes,
                                                    &organisation_length);
    if (render != SAU_OK) {
        sau_error_report_generic("Hata: arşiv organizasyon bölümü hazırlanamadı.");
        return render;
    }

    SauStatus emitted = emit_archive(command, &blueprint,
                                     organisation_bytes, organisation_length);
    free(organisation_bytes);
    if (emitted != SAU_OK) {
        return emitted;
    }

    puts("Dosyalar birleştirildi.");
    return SAU_OK;
}
