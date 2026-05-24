#include "sau_args.h"

#include <stddef.h>
#include <string.h>


static void initialise_command(SauCommand *out)
{
    out->mode = SAU_MODE_NONE;
    out->output_archive = SAU_DEFAULT_ARCHIVE;
    out->archive_to_open = NULL;
    out->target_directory = NULL;
    out->input_count = 0;
    for (int i = 0; i < SAU_MAX_INPUT_FILES; ++i) {
        out->input_paths[i] = NULL;
    }
}

static SauStatus parse_build_mode(int argc, char **argv, SauCommand *out)
{
    int i = 2; 
    int seen_output = 0;

    while (i < argc) {
        const char *token = argv[i];
        if (strcmp(token, "-o") == 0) {
            if (seen_output) {
                sau_error_report_generic("Hata: -o parametresi yalnızca bir kez verilebilir.");
                return SAU_ERR_USAGE;
            }
            if (i + 1 >= argc) {
                sau_error_report_generic("Hata: -o parametresinden sonra dosya adı bekleniyor.");
                return SAU_ERR_USAGE;
            }
            out->output_archive = argv[i + 1];
            seen_output = 1;
            i += 2;
            continue;
        }
        /* -o değilse giriş dosyası yolu olarak alıyorum. */
        if (out->input_count >= SAU_MAX_INPUT_FILES) {
            sau_error_report_generic(
                "Hata: en fazla 32 giriş dosyası verilebilir.");
            return SAU_ERR_TOO_MANY_FILES;
        }
        out->input_paths[out->input_count++] = token;
        i += 1;
    }

    if (out->input_count == 0) {
        sau_error_report_generic("Hata: -b modunda en az bir giriş dosyası gerekir.");
        return SAU_ERR_USAGE;
    }
    return SAU_OK;
}

static SauStatus parse_extract_mode(int argc, char **argv, SauCommand *out)
{
    /* Şartname -a sonrası en fazla 2 token istiyor: arşiv + opsiyonel dizin. */
    int leftover = argc - 2;
    if (leftover < 1) {
        sau_error_report_generic("Hata: -a modunda arşiv dosyası adı gerekir.");
        return SAU_ERR_USAGE;
    }
    if (leftover > 2) {
        sau_error_report_generic(
            "Hata: -a parametresinden sonra en fazla iki parametre verilebilir.");
        return SAU_ERR_USAGE;
    }
    out->archive_to_open = argv[2];
    if (leftover == 2) {
        out->target_directory = argv[3];
    }
    return SAU_OK;
}

SauStatus sau_args_parse(int argc, char **argv, SauCommand *out)
{
    if (out == NULL) {
        return SAU_ERR_USAGE;
    }
    initialise_command(out);

    if (argc < 2) {
        sau_error_print_usage_hint();
        return SAU_ERR_USAGE;
    }

    if (strcmp(argv[1], "-b") == 0) {
        out->mode = SAU_MODE_BUILD;
        return parse_build_mode(argc, argv, out);
    }
    if (strcmp(argv[1], "-a") == 0) {
        out->mode = SAU_MODE_EXTRACT;
        return parse_extract_mode(argc, argv, out);
    }
    sau_error_print_usage_hint();
    return SAU_ERR_USAGE;
}
