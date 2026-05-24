#include <stdio.h>
#include <stdlib.h>

#include "sau_args.h"
#include "sau_error.h"
#include "sau_pack_writer.h"
#include "sau_unpack_reader.h"



static int status_to_exit_code(SauStatus status)
{
    switch (status) {
        case SAU_OK:
            return 0;
        case SAU_ERR_USAGE:
            return 64;             /* EX_USAGE — kullanım hatası           */
        case SAU_ERR_INPUT_MISSING:
        case SAU_ERR_INPUT_FORMAT:
            return 66;             /* giriş dosyası eksik/uyumsuz           */
        case SAU_ERR_TOO_MANY_FILES:
        case SAU_ERR_SIZE_LIMIT:
            return 65;             /* EX_DATAERR — veri sınırı aşıldı      */
        case SAU_ERR_ARCHIVE_CORRUPT:
            return 65;
        case SAU_ERR_PATH_UNSAFE:
            return 65;
        case SAU_ERR_TARGET_DIR:
        case SAU_ERR_IO:
        case SAU_ERR_MEMORY:
        default:
            return 74;             /* EX_IOERR — G/Ç ya da kaynak hatası   */
    }
}

int main(int argc, char **argv)
{
    SauCommand command;
    SauStatus parse_status = sau_args_parse(argc, argv, &command);
    if (parse_status != SAU_OK) {
        return status_to_exit_code(parse_status);
    }

    SauStatus action_status = SAU_OK;
    switch (command.mode) {
        case SAU_MODE_BUILD:
            action_status = sau_pack_write_bundle(&command);
            break;
        case SAU_MODE_EXTRACT:
            action_status = sau_unpack_restore_bundle(&command);
            break;
        case SAU_MODE_NONE:
        default:
            sau_error_print_usage_hint();
            action_status = SAU_ERR_USAGE;
            break;
    }
    return status_to_exit_code(action_status);
}
