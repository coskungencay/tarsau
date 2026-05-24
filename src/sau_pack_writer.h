#ifndef SAU_PACK_WRITER_H
#define SAU_PACK_WRITER_H

#include "sau_args.h"
#include "sau_error.h"

/*
 * Build (-b) akışının giriş noktası. Ayrıştırılmış komutu alır, her
 * giriş yolunu gezer, ASCII doğrulaması ve toplam boyut kontrolü yapar,
 * organizasyon bölümünü üretir ve son .sau dosyasını atomik biçimde
 * diske yazar.
 *
 * Çıktı davranışı:
 *
 *   - Başarılı olduğunda stdout'a `Dosyalar birleştirildi.` yazar ve
 *     SAU_OK döner.
 *   - İlk ASCII olmayan giriş dosyasında şartnamenin istediği
 *     `<dosya> giriş dosyasının formatı uyumsuzdur!` mesajını yazar ve
 *     yarım arşiv bırakmadan SAU_ERR_INPUT_FORMAT döner.
 *   - Diğer hatalarda kısa Türkçe açıklama basar.
 */
SauStatus sau_pack_write_bundle(const SauCommand *command);

#endif /* SAU_PACK_WRITER_H */
