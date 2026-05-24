#ifndef SAU_BLUEPRINT_H
#define SAU_BLUEPRINT_H

#include <stddef.h>
#include <sys/types.h>

#include "sau_args.h"
#include "sau_error.h"
#include "sau_path_safety.h"

/*
 * Her .sau dosyasının başındaki sabit uzunluklu organizasyon-boyut
 * başlığının byte cinsinden genişliği. Renderer ile parser'ı
 * birbirine bağlayan değer olduğu için buraya koydum; değiştirmek
 * istersek tek yerden bakılır.
 */
#define SAU_HEADER_WIDTH 10

#ifndef SAU_MAX_TOTAL_BYTES
#define SAU_MAX_TOTAL_BYTES (200LL * 1024LL * 1024LL)
#endif

/*
 * Organizasyon bölümündeki tek bir arşiv kaydı. `stored_name`,
 * sau_path_safety tarafından temizlenmiş güvenli bir basename tutar.
 * `permission_bits` st_mode'un alt 9 bitini (rwxrwxrwx) tutar; diske
 * yazılırken 4 haneli octal string olarak serileştirilir.
 * `byte_count` bu kaydın okunup yazılacağı tam payload uzunluğu.
 */
typedef struct {
    char       stored_name[SAU_MAX_STORED_NAME + 1];
    mode_t     permission_bits;
    long long  byte_count;
} SauEntry;

/*
 * Blueprint, organizasyon bölümünün bellek karşılığı. 32 giriş
 * * birkaç yüz byte boyutunda, küçük. Bu yüzden heap'e atmak yerine
 * build/extract akışlarında stack'te tutuyorum.
 */
typedef struct {
    SauEntry   entries[SAU_MAX_INPUT_FILES];
    int        count;
    long long  payload_total_bytes;
} SauBlueprint;

/*
 * Blueprint'i sıfırlar.
 */
void sau_blueprint_reset(SauBlueprint *blueprint);

/*
 * Bir kayıt ekler. Çağıran taraf güvenli basename'i daha önce
 * sau_path_make_stored_basename() ile üretmiş olmalı. Slot sayısı
 * taşarsa SAU_ERR_TOO_MANY_FILES döner.
 */
SauStatus sau_blueprint_append(SauBlueprint *blueprint,
                               const char   *stored_name,
                               mode_t        permission_bits,
                               long long     byte_count);


SauStatus sau_blueprint_render_records(const SauBlueprint *blueprint,
                                       char              **out_buffer,
                                       size_t             *out_length);

SauStatus sau_blueprint_parse_records(SauBlueprint *blueprint,
                                      const char   *slice,
                                      size_t        length);

#endif /* SAU_BLUEPRINT_H */
