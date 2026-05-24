#ifndef SAU_ARGS_H
#define SAU_ARGS_H

#include "sau_error.h"

/*
 * Build modunda kabul edilecek giriş dosyası sayısının üst sınırı.
 * Şartname 32 dedi, ben de bu sabiti buraya koydum ki başka modüller
 * de aynı sayıya referans verebilsin.
 */
#define SAU_MAX_INPUT_FILES 32

/* -o verilmediğinde üretilecek varsayılan arşiv adı. */
#define SAU_DEFAULT_ARCHIVE "a.sau"

/* Extract modunda beklenen arşiv dosyası uzantısı. */
#define SAU_EXTENSION ".sau"

typedef enum {
    SAU_MODE_NONE = 0,
    SAU_MODE_BUILD,
    SAU_MODE_EXTRACT
} SauMode;

/*
 * Komut satırının ayrıştırılmış hali. Parser dosya sistemine
 * dokunmuyor, "dosya var mı yok mu" gibi şeyleri kontrol etmiyor —
 * o iş build/extract modüllerinin. String pointer'ları doğrudan
 * argv'yi gösterdiği için SauCommand'in ömrü main() ile sınırlı.
 */
typedef struct {
    SauMode      mode;
    const char  *output_archive;        /* -b: üretilecek arşiv (yoksa default)  */
    const char  *archive_to_open;       /* -a: okunacak arşiv                    */
    const char  *target_directory;      /* -a: opsiyonel hedef dizin, NULL olabilir */
    const char  *input_paths[SAU_MAX_INPUT_FILES];
    int          input_count;
} SauCommand;

/*
 * argv'yi gezer ve SauCommand'i doldurur.
 *
 * Başarılıysa SAU_OK döner. Hata durumunda SAU_ERR_USAGE veya
 * SAU_ERR_TOO_MANY_FILES döner ve sau_error üzerinden ipucu basar.
 */
SauStatus sau_args_parse(int argc, char **argv, SauCommand *out);

#endif /* SAU_ARGS_H */
