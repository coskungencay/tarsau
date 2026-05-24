#ifndef SAU_UNPACK_READER_H
#define SAU_UNPACK_READER_H

#include "sau_args.h"
#include "sau_error.h"

/*
 * Extract (-a) akışının giriş noktası.
 *
 *   - Başarılı olduğunda, hedef dizin verilmişse
 *       `<hedef> dizininde a, b ve c dosyaları açıldı.`
 *     verilmemişse
 *       `Geçerli dizinde a, b ve c dosyaları açıldı.`
 *     yazılır.
 *
 *   - Arşivde yapısal bir problem çıkarsa
 *       `Arşiv dosyası uygunsuz veya bozuk!`
 *     yazılır ve SAU_ERR_ARCHIVE_CORRUPT döner. Yarım kalan dosya bırakılmaz.
 */
SauStatus sau_unpack_restore_bundle(const SauCommand *command);

#endif /* SAU_UNPACK_READER_H */
